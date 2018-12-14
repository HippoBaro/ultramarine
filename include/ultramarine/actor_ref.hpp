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

#include <variant>
#include <core/reactor.hh>
#include "actor.hpp"
#include "impl/actor_ref_impl.hpp"

namespace ultramarine {

    template<typename Actor, ActorKind = actor_kind<Actor>()>
    class actor_ref {
    };

    template<typename Actor>
    class actor_ref<Actor, ActorKind::SingletonActor> {
        impl::actor_ref_variant<Actor> impl;
    public:

        template<typename KeyType>
        explicit constexpr actor_ref(KeyType &&key) :
                impl(impl::make_actor_ref_impl<Actor>(std::forward<KeyType>(key))) {}

        explicit constexpr actor_ref(impl::local_actor_ref<Actor> const &ref) : impl(ref) {};

        explicit constexpr actor_ref(impl::collocated_actor_ref<Actor> const &ref) : impl(ref) {};

        constexpr actor_ref(actor_ref const &) = default;

        constexpr actor_ref(actor_ref &&) noexcept = default;

        template<typename Func>
        inline constexpr auto visit(Func &&func) const noexcept {
            return std::visit([func = std::forward<Func>(func)](auto const &impl) mutable {
                return func(impl);
            }, impl);
        }

        template<typename Handler, typename ...Args>
        constexpr auto inline tell(Handler message, Args &&... args) const {
            return [this, message, args = std::make_tuple(std::forward<Args>(args) ...)]() mutable {
                return visit([message, &args](auto &&impl) {
                    return std::apply([&impl, message](Args &&... args) {
                        return impl.tell(message, std::forward<Args>(args) ...);
                    }, std::move(args));
                });
            }();
        }

        template<typename Handler>
        constexpr auto inline tell(Handler message) const {
            return visit([message](auto &&impl) {
                return impl.tell(message);
            });
        };
    };

    template<typename Actor>
    class actor_ref<Actor, ActorKind::LocalActor> {
        ActorKey<Actor> key;

    public:

        template<typename KeyType>
        explicit constexpr actor_ref(KeyType &&key) : key(std::forward<KeyType>(key)) {}

        constexpr actor_ref(actor_ref const &) = default;

        constexpr actor_ref(actor_ref &&) noexcept = default;

        template<typename Func>
        inline constexpr auto visit(Func &&func) const noexcept {
            std::size_t next = 0;

            if constexpr (std::is_base_of_v<local_actor<>, Actor>) {
                next = (Actor::round_robin_counter++ + seastar::engine().cpu_id()) % seastar::smp::count;
            } else {
                next = (Actor::round_robin_counter++ + seastar::engine().cpu_id())
                       % (seastar::smp::count < Actor::max_activations ? seastar::smp::count : Actor::max_activations);
            }

            if (next == seastar::engine().cpu_id()) {
                return func(impl::local_actor_ref<Actor>(key, impl::actor_directory<Actor>::hash_key(key)));
            } else {
                return func(impl::collocated_actor_ref<Actor>(key, impl::actor_directory<Actor>::hash_key(key), next));
            }
        }

        template<typename Handler, typename ...Args>
        constexpr auto inline tell(Handler message, Args &&... args) const {
            return [this, message, args = std::make_tuple(std::forward<Args>(args) ...)]() mutable {
                return visit([message, &args](auto &&impl) {
                    return std::apply([&impl, message](Args &&... args) {
                        return impl.tell(message, std::forward<Args>(args) ...);
                    }, std::move(args));
                });
            }();
        }

        template<typename Handler>
        constexpr auto inline tell(Handler message) const {
            return visit([message](auto &&impl) {
                return impl.tell(message);
            });
        };
    };

    template<typename Actor, typename KeyType>
    [[nodiscard]] constexpr inline actor_ref<Actor> get(KeyType &&key) noexcept {
        static_assert(std::is_constructible<ActorKey<Actor>,
                KeyType &&>::value, "The provided key is not compatible with the Actor");
        return actor_ref<Actor>(std::forward<KeyType>(key));
    }
}
