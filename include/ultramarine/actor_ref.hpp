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

#include <any>
#include <variant>
#include "actor.hpp"
#include "impl/directory.hpp"
#include "impl/actor_ref_impl.hpp"

namespace ultramarine {

    /// A movable and copyable reference to a virtual actor
    /// \unique_name ultramarine::actor_ref
    /// \tparam Actor The type of [ultramarine::actor]() to reference
    /// \exclude
    template<typename Actor, ActorKind = actor_kind<Actor>()>
    class actor_ref {
    };

    /// A movable and copyable reference to a virtual actor
    /// \tparam Actor The type of [ultramarine::actor]() to reference
    /// \requires Type `Actor` shall inherit from [ultramarine::actor]()
    template<typename Actor>
    class actor_ref<Actor, ActorKind::SingletonActor> {
        const impl::actor_ref_variant<Actor> impl;
    public:

        template<typename KeyType>
        explicit constexpr actor_ref(KeyType key) :
                impl(impl::wrap_actor_ref_impl<Actor>(std::forward<KeyType>(key))) {}

        constexpr actor_ref(actor_ref const &) noexcept = default;

        constexpr actor_ref(actor_ref &&) noexcept = default;

        /// Obtain the concrete [ultramarine::actor_ref]() implementation
        /// \param func A lambda to execute with the [ultramarine::actor_ref]() implementation
        /// \returns The value returned by the provided lambda, if any
        template<typename Func>
        inline constexpr auto visit(Func &&func) const noexcept {
            return std::visit([func = std::forward<Func>(func)](auto const &impl) {
                return func(impl);
            }, impl);
        }

        /// Enqueue a message to the [ultramarine::actor]() referenced by this [ultramarine::actor_ref]() instance
        /// \effects Creates the [ultramarine::actor]() if it doesn't exist
        /// \param message The message handler to enqueue
        /// \returns A future representing the eventually returned value by the actor, or a failed future
        template<typename Handler, typename ...Args>
        constexpr auto inline tell(Handler message, Args &&... args) const {
            return [this, message, args = std::make_tuple(std::forward<Args>(args) ...)]() mutable {
                return visit([message, &args](auto const &impl) {
                    return std::apply([&impl, message](auto &&... args) {
                        return impl.tell(message, std::forward<Args>(args) ...);
                    }, std::move(args));
                });
            }();
        }

        /// Enqueue a message to the [ultramarine::actor]() referenced by this [ultramarine::actor_ref]() instance
        /// \effects Creates the [ultramarine::actor]() if it doesn't exist
        /// \param message The message handler to enqueue
        /// \returns A future representing the eventually returned value by the actor, or a failed future
        template<typename Handler>
        constexpr auto inline tell(Handler message) const {
            return visit([message](auto const &impl) {
                return impl.tell(message);
            });
        };
    };

    /// A movable and copyable reference to an [ultramarine::actor]()
    /// \tparam Actor The type of [ultramarine::actor]() to reference
    /// \requires Type `Actor` shall inherit from [ultramarine::actor]() and from attribute [ultramarine::local_actor]()
    template<typename Actor>
    class actor_ref<Actor, ActorKind::LocalActor> {
        const impl::ActorKey<Actor> key;

    public:

        template<typename KeyType>
        explicit constexpr actor_ref(KeyType key) : key(std::forward<KeyType>(key)) {}

        constexpr actor_ref(actor_ref const &) noexcept = default;

        constexpr actor_ref(actor_ref &&) noexcept = default;

        /// Obtain the concrete [ultramarine::actor_ref]() implementation
        /// \param func A lambda to execute with the [ultramarine::actor_ref]() implementation
        /// \returns The value returned by the provided lambda, if any
        template<typename Func>
        inline constexpr auto visit(Func &&func) const noexcept {
            seastar::shard_id next = 0;

            if constexpr (is_unlimited_concurrent_local_actor_v<Actor>) {
                next = (Actor::round_robin_counter++ + seastar::engine().cpu_id()) % seastar::smp::count;
            } else {
                next = (Actor::round_robin_counter++ + seastar::engine().cpu_id())
                       % (seastar::smp::count < Actor::max_activations ? seastar::smp::count : Actor::max_activations);
            }

            return impl::do_with_actor_ref_impl<Actor, impl::ActorKey<Actor>>(key, next, [&func](auto const &impl) {
                return func(impl);
            });
        }

        /// Enqueue a message to the [ultramarine::actor]() referenced by this [ultramarine::actor_ref]() instance
        /// \effects Creates the [ultramarine::actor]() if it doesn't exist
        /// \param message The message handler to enqueue
        /// \param args Arguments to pass to the message handler
        /// \returns A future representing the eventually returned value by the actor, or a failed future
        template<typename Handler, typename ...Args>
        constexpr auto inline tell(Handler message, Args &&... args) const {
            return [this, message, args = std::make_tuple(std::forward<Args>(args) ...)]() mutable {
                return visit([message, &args](auto const &impl) {
                    return std::apply([&impl, message](auto &&... args) {
                        return impl.tell(message, std::forward<Args>(args) ...);
                    }, std::move(args));
                });
            }();
        }

        /// Enqueue a message to the [ultramarine::actor]() referenced by this [ultramarine::actor_ref]() instance
        /// \effects Creates the [ultramarine::actor]() if it doesn't exist
        /// \param message The message handler to enqueue
        /// \returns A future representing the eventually returned value by the actor, or a failed future
        template<typename Handler>
        constexpr auto inline tell(Handler message) const {
            return visit([message](auto const &impl) {
                return impl.tell(message);
            });
        };
    };

    /// A movable and copyable polymorphic reference to a virtual actor.
    /// Useful when an [ultramarine::actor]() declares a message with an actor_ref<itself> as argument.
    /// Avoids incomplete type compiler error.
    /// \unique_name ultramarine::poly_actor_ref
    /// \exclude
    class poly_actor_ref {
        std::any opaque;

    public:
        template<template<typename> class Ref, typename Actor>
        constexpr poly_actor_ref(Ref<Actor> &&ref) noexcept : opaque(std::forward<Ref<Actor>>(ref)) {
            static_assert(std::is_same_v<actor_ref<Actor>,
                    Ref < Actor>>, "poly_actor_ref used with non [ultramarine::actor_ref]() type");
        };

        template<template<typename> class Ref, typename Actor>
        constexpr poly_actor_ref(Ref<Actor> const &ref) noexcept : opaque(ref) {
            static_assert(std::is_same_v<actor_ref<Actor>, Ref<Actor>>,
                          "poly_actor_ref used with non [ultramarine::actor_ref]() type");
        };

        /// Cast this instance into a fully specified actor_ref
        /// \requires Actor shall be of type [ultramarine::actor]()
        /// \tparam Actor The type of [ultramarine::actor]() to reference
        /// \returns An [ultramarine::actor_ref]()
        template<typename Actor>
        auto as() {
            return std::any_cast<actor_ref<Actor>>(opaque);
        }
    };

    /// Create a reference to a virtual actor
    /// \tparam Actor The type of [ultramarine::actor]() to reference
    /// \requires Type `Actor` shall inherit from [ultramarine::actor]()
    /// \param key The primary key of the actor
    /// \returns An [ultramarine::actor_ref]()
    template<typename Actor, typename KeyType = typename Actor::Keytype>
    [[nodiscard]] constexpr inline actor_ref<Actor> get(KeyType &&key) noexcept {
        return actor_ref<Actor>(std::forward<KeyType>(key));
    }
}
