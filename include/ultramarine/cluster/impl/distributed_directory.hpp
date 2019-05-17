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

#include "ultramarine/impl/directory.hpp"
#include "node.hpp"
#include "message_serializer.hpp"
#include "membership.hpp"

namespace ultramarine::cluster::impl {

    template<typename T>
    using ActorKey = ultramarine::impl::ActorKey<T>;

    template<typename Actor>
    struct directory {
        [[nodiscard]] static constexpr node const *hold_remote_peer(ActorKey<Actor> const &key, std::size_t hash) {
            return membership::service.local().node_for_key(hash);
        }

        template<typename Ret, typename Class, typename ...FArgs, typename ...Args>
        static constexpr auto
        dispatch_message(node const &n, ActorKey<Actor> const &key, Ret (Class::*fptr)(FArgs...) const, uint32_t id,
                         Args &&... args) {
            return n.rpc->make_client<Ret(ActorKey<Actor>, FArgs...)>(id)(*n.client, key, std::forward<Args>(args) ...);
        }

        template<typename Ret, typename Class, typename ...FArgs, typename ...Args>
        static constexpr auto
        dispatch_message(node const &n, ActorKey<Actor> const &key, Ret (Class::*fptr)(FArgs...), uint32_t id,
                         Args &&... args) {
            return n.rpc->make_client<Ret(ActorKey<Actor>, FArgs...)>(id)(*n.client, key, std::forward<Args>(args) ...);
        }

        template<typename Ret, typename Class, typename ...FArgs, typename PackedArgs>
        static constexpr auto
        dispatch_packed_message(node const &n, ActorKey<Actor> const &key, Ret (Class::*fptr)(FArgs...) const,
                                uint32_t id, PackedArgs &&args) {
            using FutReturn = seastar::futurize_t<std::result_of_t<decltype(fptr)(Actor, FArgs...)>>;
            using ReturnType = typename ultramarine::impl::get0_return_type<typename FutReturn::value_type>::type;
            if constexpr (std::is_same_v<ReturnType, void>) {
                auto client = n.rpc->make_client<seastar::future<>(ActorKey<Actor>, PackedArgs)>(id | (1U << 0U));
                return client(*n.client, key, std::forward<PackedArgs>(args));
            } else {
                auto client = n.rpc->make_client<seastar::future<std::vector<ReturnType>>(ActorKey<Actor>, PackedArgs)>(
                        id | (1U << 0U));
                return client(*n.client, key, std::forward<PackedArgs>(args));
            }

        }

        template<typename Ret, typename Class, typename ...FArgs, typename PackedArgs>
        static constexpr auto
        dispatch_packed_message(node const &n, ActorKey<Actor> const &key, Ret (Class::*fptr)(FArgs...), uint32_t id,
                                PackedArgs &&args) {
            using FutReturn = seastar::futurize_t<std::result_of_t<decltype(fptr)(Actor, FArgs...)>>;
            using ReturnType = typename ultramarine::impl::get0_return_type<typename FutReturn::value_type>::type;
            if constexpr (std::is_same_v<ReturnType, void>) {
                auto client = n.rpc->make_client<seastar::future<>(ActorKey<Actor>, PackedArgs)>(id | (1U << 0U));
                return client(*n.client, key, std::forward<PackedArgs>(args));
            } else {
                auto client = n.rpc->make_client<seastar::future<std::vector<ReturnType>>(ActorKey<Actor>, PackedArgs)>(
                        id | (1U << 0U));
                return client(*n.client, key, std::forward<PackedArgs>(args));
            }
        }
    };
}