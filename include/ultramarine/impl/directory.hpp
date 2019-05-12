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
            [[nodiscard]] static constexpr auto hash_key(KeyType const& key) noexcept {
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
            static constexpr auto dispatch_message(Actor *activation, Handler message, Args &&... args) {
                if constexpr (is_reentrant_v<Actor>) {
                    return (activation->*vtable<Actor>::table[message])(std::forward<Args>(args) ...);
                } else {
                    return seastar::with_semaphore(activation->semaphore, 1, std::chrono::seconds(1),
                                                   [message, activation, args = std::make_tuple(
                                                           std::forward<Args>(args) ...)] () mutable {
                                                       return std::apply([activation, message](auto &&... args) {
                                                           return (activation->*vtable<Actor>::table[message])(
                                                                   std::forward<Args>(args) ...);
                                                       }, std::move(args));
                                                   });
                }
            }

            template<typename KeyType, typename Handler, typename ...Args>
            static constexpr auto dispatch_message(KeyType &&key, actor_id id, Handler message, Args &&... args) {
                return dispatch_message(hold_activation(std::forward<KeyType>(key), id), message,
                                        std::forward<Args>(args) ...);
            }

            template<typename KeyType, typename Handler, typename ...Args>
            static constexpr auto dispatch_packed_message(KeyType &&key, actor_id id, Handler message,
                                                          std::vector<std::tuple<Args...>> &&args) {
                using FutReturn = seastar::futurize_t<std::result_of_t<decltype(vtable<Actor>::table[message])(Actor, Args...)>>;
                using ReturnType = typename FutReturn::value_type;

                auto *activation = hold_activation(std::forward<KeyType>(key), id);
                std::vector<ReturnType> ret;
                ret.reserve(std::size(args));

                return seastar::do_with(activation, std::move(ret), std::move(args),
                        [key = std::forward<KeyType>(key), id, message]
                        (auto* activation, auto &ret, auto &targs) mutable {
                    return seastar::parallel_for_each(boost::irange(0UL, targs.size()),
                            [key = std::forward<KeyType>(key), activation, id, message, &ret, &targs]
                            (std::size_t i) mutable {
                        return std::apply([key = std::forward<KeyType>(key), activation, id, message, &ret, i]
                        (auto &&... args) mutable {
                            return dispatch_message(activation, message, std::forward<Args>(args) ...)
                            .then_wrapped([&ret] (auto &&fut) {
                                if (fut.failed()) {
                                    return seastar::make_exception_future(fut.get_exception());
                                }
                                ret.emplace_back(std::move(fut.get0()));
                                return seastar::make_ready_future();
                            });
                            return seastar::make_ready_future();
                        }, std::move(targs[i]));
                    }).then([&ret] {
                        return std::move(ret);
                    });
                });
            }
        };
    }
}