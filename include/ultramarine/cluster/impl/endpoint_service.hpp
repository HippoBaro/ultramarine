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

#include "global_message_handlers.hpp"
#include "message_serializer.hpp"

namespace ultramarine::cluster {
    /// \exclude
    struct exported_actor_endpoints_service {
    private:
        rpc_proto proto{serializer{}};
        std::unique_ptr<rpc_proto::server> server;

    public:
        exported_actor_endpoints_service(seastar::ipv4_addr const& bind_to) {
            for (const auto &handler : impl::remote_init_handlers()) {
                handler.second(&proto);
            }
            proto.set_logger([](const seastar::sstring &log) {
                fmt::print("RPCS -> {}\n", log);
            });
            server = std::make_unique<rpc_proto::server>(proto, bind_to);
        }

        // needed by seastar::sharded<T>
        seastar::future<> stop() {
            return server->stop();
        }
    };

    /// \exclude
    struct imported_actor_endpoints_service {
    private:
        rpc_proto proto{serializer{}};
        std::unique_ptr<rpc_proto::client> client;

    public:

        imported_actor_endpoints_service() {
            for (const auto &handler : impl::remote_init_handlers()) {
                handler.second(&proto);
            }
            proto.set_logger([](const seastar::sstring &log) {
                fmt::print("RPCC -> {}\n", log);
            });

            rpc_proto::server *t;
        }

        template<typename ActorKey, typename Ret, typename Class, typename ...Args>
        constexpr auto remote_dispatch(ActorKey const &key, Ret (Class::*fptr)(Args...) const, uint32_t id) {
            return proto.make_client<Ret(ActorKey, Args...)>(id)(*client, key);
        }

        template<typename ActorKey, typename Ret, typename Class, typename ...FArgs, typename ...Args>
        constexpr auto
        remote_dispatch(ActorKey const &key, Ret (Class::*fptr)(FArgs...) const, uint32_t id, Args &&... args) {
            return proto.make_client<Ret(ActorKey, FArgs...)>(id)(*client, key, std::forward<Args>(args) ...);
        }

        template<typename ActorKey, typename Ret, typename Class, typename ...Args>
        constexpr auto remote_dispatch(ActorKey const &key, Ret (Class::*fptr)(Args...), uint32_t id) {
            return proto.make_client<Ret(ActorKey, Args...)>(id)(*client, key);
        }

        template<typename ActorKey, typename Ret, typename Class, typename ...FArgs, typename ...Args>
        constexpr auto
        remote_dispatch(ActorKey const &key, Ret (Class::*fptr)(FArgs...), uint32_t id, Args &&... args) {
            return proto.make_client<Ret(ActorKey, FArgs...)>(id)(*client, key, std::forward<Args>(args) ...);
        }

        seastar::future<> connect(seastar::socket_address addr) {
            client = std::make_unique<rpc_proto::client>(proto, addr);
            return client->await_connection();
        }

        // needed by seastar::sharded<T>
        seastar::future<> stop() {
            if (client) {
                return client->stop();
            }
            return seastar::make_ready_future();
        }
    };
}