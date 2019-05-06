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

#include <seastar/core/semaphore.hh>

namespace ultramarine {

    ///\exclude
    namespace impl {
        struct local_actor {
        };
    }

    /// Actor attribute base class that specify that the Derived actor should be treated as a local actor
    /// \unique_name ultramarine::local_actor
    /// \requires Type `Derived` shall inherit from [ultramarine::actor]()
    /// \tparam Derived The derived [ultramarine::actor]() class for CRTP purposes
    /// \tparam ConcurrencyLimit Optional. The limit of concurrent local activations for this actor
    template<typename Derived, std::size_t ConcurrencyLimit = std::numeric_limits<std::size_t>::max()>
    struct local_actor : impl::local_actor {
        static_assert(ConcurrencyLimit > 0, "Local actor concurrency limit must be a positive integer");

        /// \exclude
        static constexpr std::size_t max_activations = ConcurrencyLimit;

        /// \exclude
        static thread_local std::size_t round_robin_counter;
    };

    /// \exclude
    template<typename Derived, std::size_t ConcurrencyLimit>
    thread_local std::size_t local_actor<Derived, ConcurrencyLimit>::round_robin_counter = 0;

    /// Actor attribute base class that specify that the Derived actor should be protected against reentrancy
    /// \unique_name ultramarine::non_reentrant_actor
    /// \requires Type `Derived` shall inherit from [ultramarine::actor]()
    /// \tparam Derived The derived actor class for CRTP purposes
    template <typename Derived>
    struct non_reentrant_actor {
        /// \exclude
        seastar::semaphore semaphore = seastar::semaphore(1);
    };

    /// Enum representing the possible kinds of [ultramarine::actor]()
    /// \unique_name ultramarine::actor_type
    enum class ActorKind {
        SingletonActor,
        LocalActor
    };

    /// Get the [ultramarine::actor]() type
    /// \tparam Actor The [ultramarine::actor]() type to test against
    /// \requires Type `Actor` shall inherit from [ultramarine::actor]()
    /// \returns An enum value of type [ultramarine::actor_type]()
    template <typename Actor>
    constexpr ActorKind actor_kind() {
        if constexpr (std::is_base_of_v<impl::local_actor, Actor>) {
            return ActorKind::LocalActor;
        }
        return ActorKind::SingletonActor;
    }

    /// Compile-time trait testing if the [ultramarine::actor]() type is reentrant
    /// \requires Type `Actor` shall inherit from [ultramarine::actor]()
    /// \tparam Actor The [ultramarine::actor]() type to test against
    /// \returns `true` if type `Actor` is reentrant, `false` otherwise
    template<typename Actor>
    constexpr bool is_reentrant_v = !std::is_base_of_v<non_reentrant_actor<Actor>, Actor>;

    /// Compile-time trait testing if the [ultramarine::actor]() type is local
    /// \requires Type `Actor` shall inherit from [ultramarine::actor]()
    /// \tparam Actor The actor type to test against
    /// \returns `true` if type `Actor` is local, `false` otherwise
    template<typename Actor>
    constexpr bool is_local_actor_v = std::is_base_of_v<impl::local_actor, Actor>;

    /// Compile-time trait testing if the [ultramarine::local_actor]() type doesn't specify a concurrency limit
    /// \requires Type `Actor` shall inherit from [ultramarine::local_actor]()
    /// \tparam Actor The actor type to test against
    /// \returns `true` if `Actor` has no concurrency limit, `false` otherwise
    template<typename Actor>
    constexpr bool is_unlimited_concurrent_local_actor_v = std::is_base_of_v<local_actor<Actor>, Actor>;
}