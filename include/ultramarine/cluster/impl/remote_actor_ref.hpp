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

#include <seastar/core/future.hh>
#include <seastar/core/reactor.hh>
#include <ultramarine/impl/directory.hpp>
#include "service.hpp"
#include "node.hpp"

namespace ultramarine::cluster::impl {
    template<typename Actor>
    class remote_actor_ref {
        ultramarine::impl::ActorKey<Actor> key;
        std::size_t hash;
        ultramarine::cluster::node loc;
        imported_actor_endpoints_service &client;

    public:
        using ActorType = Actor;

        explicit constexpr remote_actor_ref(ultramarine::impl::ActorKey<Actor> k, std::size_t hash,
                                            ultramarine::cluster::node &&loc) :
                key(std::move(k)), hash(hash), loc(std::move(loc)),
                client(ultramarine::cluster::service().import()) {}

        constexpr remote_actor_ref(remote_actor_ref const &) = default;

        constexpr remote_actor_ref(remote_actor_ref &&) noexcept = default;

        inline constexpr typename Actor::internal::template interface<remote_actor_ref<Actor>> operator->() const {
            return typename Actor::internal::template interface<remote_actor_ref<Actor>>{*this};
        }

        template<typename Handler>
        inline constexpr auto tell(Handler message) const {
            return client.remote_dispatch(key, ultramarine::impl::vtable<Actor>::table[message], message.value);
        }

        template<typename Handler, typename ...Args>
        inline constexpr auto tell(Handler message, Args &&... args) const {
            return client.remote_dispatch(key, ultramarine::impl::vtable<Actor>::table[message], message.value(),
                                          std::forward<Args>(args) ...);
        }
    };
}