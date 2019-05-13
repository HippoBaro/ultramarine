/*
 * MIT License
 *
 * Copyright (c) 2018 Hippolyte Barraud
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <seastar/core/reactor.hh>
#include <ultramarine/impl/actor_traits.hpp>

namespace ultramarine {

    /// [ultramarine::actor]() are identified internally via an unsigned integer id
    /// \unique_name ultramarine::actor_id
    using actor_id = std::size_t;

    namespace impl {
        /// A round-robin placement strategy that shards actors based on the modulo of their [ultramarine::actor::KeyType]()
        /// \unique_name ultramarine::round_robin_local_placement_strategy
        struct round_robin_local_placement_strategy {
            /// \param A hashed [ultramarine::actor::KeyType]()
            /// \returns The location the actor should be placed in
            seastar::shard_id operator()(std::size_t hash) const noexcept {
                return hash % seastar::smp::count;
            }
        };

        /// Default local placement strategy uses [ultramarine::round_robin_local_placement_strategy]()
        /// \unique_name ultramarine::default_local_placement_strategy
        using default_local_placement_strategy = round_robin_local_placement_strategy;

        using actor_activation_id = unsigned int;

        template<typename Actor>
        using directory = std::unordered_map<actor_id, Actor>;

        template<typename Actor>
        using ActorKey = typename Actor::KeyType;

        template<typename Actor>
        struct vtable {
            static constexpr auto table = Actor::internal::message::make_vtable();
        };

        template<typename Actor>
        struct actor_directory {

            template<typename KeyType>
            [[nodiscard]] static constexpr auto hash_key(KeyType const &key) noexcept {
                return std::hash<ActorKey<Actor>>{}(key);
            }

            [[nodiscard]] static constexpr Actor *hold_activation(ActorKey<Actor> &&key, actor_id id) {
                if (!Actor::directory) {
                    Actor::directory = std::make_unique<ultramarine::impl::directory<Actor>>();
                }

                auto r = std::get<0>(Actor::directory->try_emplace(id, std::forward<ActorKey<Actor>>(key)));
                return &(std::get<1>(*r));
            }

            template<typename Handler, typename ...Args>
            static constexpr auto dispatch_message_impl(Actor *activation, Handler message, Args &&... args) {
                if constexpr (is_reentrant_v<Actor>) {
                    return (activation->*vtable<Actor>::table[message])(std::forward<Args>(args) ...);
                } else {
                    return seastar::with_semaphore(activation->semaphore, 1, std::chrono::seconds(1),
                                                   [message, activation, args = std::make_tuple(
                                                           std::forward<Args>(args) ...)]() mutable {
                                                       return std::apply([activation, message](auto &&... args) {
                                                           return (activation->*vtable<Actor>::table[message])(
                                                                   std::forward<Args>(args) ...);
                                                       }, std::move(args));
                                                   });
                }
            }

            template<typename KeyType, typename Handler, typename ...Args>
            static constexpr auto dispatch_message(KeyType &&key, actor_id id, Handler message, Args &&... args) {
                return dispatch_message_impl(hold_activation(std::forward<KeyType>(key), id), message,
                                             std::forward<Args>(args) ...);
            }

            template<typename ReturnType, typename KeyType, typename Handler, typename ...Args>
            static constexpr std::enable_if_t<std::is_same_v<ReturnType, void>, seastar::future<>>
            dispatch_packed_message(Actor *act, KeyType &&key, actor_id id, Handler message,
                                    std::vector<std::tuple<Args...>> &&args) {
                using namespace seastar;
                using FuncPtr = decltype(&dispatch_message_impl<Handler, Args...>);

                return do_with(act, std::move(args), [id, message](auto *act, auto &targs) mutable {
                    return parallel_for_each(targs, [act, id, message, &targs](auto &targ) mutable {
                        return std::apply([act, id, message](auto &&... args) mutable {
                            return futurize<ReturnType>::template apply<FuncPtr>(&dispatch_message_impl, act, message,
                                                                                 std::forward<Args>(args)...);
                        }, std::move(targ));
                    });
                });
            }

            template<typename ReturnType, typename KeyType, typename Handler, typename ...Args>
            static constexpr std::enable_if_t<!std::is_same_v<ReturnType, void>, seastar::future<std::vector<ReturnType>>>
            dispatch_packed_message(Actor *act, KeyType &&key, actor_id id, Handler message,
                                    std::vector<std::tuple<Args...>> &&args) {
                using namespace seastar;
                using FuncPtr = decltype(&dispatch_message_impl<Handler, Args...>);

                std::vector<ReturnType> ret;
                ret.reserve(std::size(args));
                return do_with(act, std::move(ret), std::move(args), [id, message]
                        (auto *act, auto &ret, auto &targs) mutable {
                    return parallel_for_each(targs, [act, id, message, &ret, &targs](auto &targ) mutable {
                        return std::apply([act, id, message, &ret](auto &&... args) mutable {
                            auto f = futurize<ReturnType>::template apply<FuncPtr>(&dispatch_message_impl, act, message,
                                                                                   std::forward<Args>(args)...);
                            return f.then_wrapped([&ret](auto &&fut) {
                                if (fut.failed()) { return make_exception_future(fut.get_exception()); }
                                ret.emplace_back(std::move(fut.get0()));
                                return make_ready_future();
                            });
                        }, std::move(targ));
                    }).then([&ret] { return std::move(ret); });
                });
            }

            template<typename ... T>
            struct get0_return_type {
                using type = void;

                static type get0(std::tuple<T...> v) {}
            };

            template<class T0, class ... T>
            struct get0_return_type<std::tuple<T0, T...>> {
                using type = T0;

                static type get0(std::tuple<T0, T...> v) { return std::get<0>(std::move(v)); }
            };

            template<typename KeyType, typename Handler, typename ...Args>
            static constexpr auto dispatch_packed_message(KeyType &&key, actor_id id, Handler message,
                                                          std::vector<std::tuple<Args...>> &&args) {
                using namespace seastar;

                using FutReturn = futurize_t<std::result_of_t<decltype(vtable<Actor>::table[message])(Actor, Args...)>>;
                using ReturnType = typename get0_return_type<typename FutReturn::value_type>::type;

                auto *act = hold_activation(std::forward<KeyType>(key), id);
                return dispatch_packed_message<ReturnType>(act, std::forward<KeyType>(key), id, message,
                                                           std::move(args));
            }
        };
    }
}