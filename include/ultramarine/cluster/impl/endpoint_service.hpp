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

#include <unordered_map>
#include "global_message_handlers.hpp"
#include "message_serializer.hpp"
#include "distributed_membership_service.hpp"
#include "handshake.hpp"

namespace ultramarine::cluster {
    /// \exclude
    struct exported_actor_endpoints_service : public event_emitter<node>,
                                              public seastar::weakly_referencable<exported_actor_endpoints_service> {
    private:
        rpc_proto proto{serializer{}};
        std::unique_ptr<rpc_proto::server> server;
        node local_node;

    public:
        exported_actor_endpoints_service(node const &local) : local_node(local) {
            for (const auto &handler : impl::remote_init_handlers()) {
                handler.second(&proto);
            }
            proto.set_logger([](const seastar::sstring &log) {
                fmt::print("RPCS -> {}\n", log);
            });

            proto.register_handler(0, [this](impl::handshake hs) {
                seastar::print("%s:%u received handshake from host %s:%u.\n", local_node.to_string(), local_node.port,
                               hs.origin.to_string(), hs.origin.port);
                for (const auto &item : hs.known_nodes) {
                    seastar::print("Handshake contains info about node %s:$u\n", item.to_string(), item.port);
                }
//                return seastar::do_with(std::move(hs), [this] (auto &hs) {
                    return raise(hs.origin); //.then([this, &hs] {
//                        //return seastar::parallel_for_each(hs.known_nodes, [this] (node const&peer) { return raise(peer); });
//                    });
//                });
            });
            server = std::make_unique<rpc_proto::server>(proto, seastar::socket_address(local.port));
        }

        // needed by seastar::sharded<T>
        seastar::future<> stop() {
            return server->stop();
        }
    };

    /// \exclude
    struct imported_actor_endpoints_service : public event_emitter<node>,
                                              public seastar::weakly_referencable<imported_actor_endpoints_service> {
    private:
        rpc_proto proto{serializer{}};
        std::unordered_map<node, std::unique_ptr<rpc_proto::client>> clients;
        std::unordered_map<node, std::unique_ptr<rpc_proto::client>> handshake_clients;
        std::unique_ptr<event_listener<node>> membership_listener = nullptr;
        node local_node;

    public:
        imported_actor_endpoints_service(node const &local,
                                         seastar::sharded<distributed_membership_service> *membership) :
                membership_listener(membership->local().make_listener()), local_node(local) {
            for (const auto &handler : impl::remote_init_handlers()) {
                handler.second(&proto);
            }
        }

        void set_event_listener(std::unique_ptr<event_listener<node>> listener) {
            membership_listener = std::move(listener);
            membership_listener->set_callback([this](node const &n) {
                if (handshake_clients.count(n) > 0) {
                    clients.insert(std::move(handshake_clients.extract(n)));
                    seastar::print("Imported remote actor interface\n");
                    return raise(n);
                }
                seastar::socket_address addr(n.addr, n.port);
                auto &client = (*handshake_clients.emplace(std::make_pair(n, std::make_unique<rpc_proto::client>(proto, addr))).first).second;
                return client->await_connection().then([this, &client, n] {
                    seastar::print("%s:%u performing handshake with host %s:%u.\n", local_node.to_string(),
                                   local_node.port, n.to_string(), n.port);
                    impl::handshake hs{local_node};
//                    std::transform(clients.begin(), clients.end(), std::back_inserter(hs.known_nodes), [] (auto const& pair) {
//                        return pair.first;
//                    });
                    return proto.make_client<void(impl::handshake)>(0)(*client, hs);
                }).then([this, n, &client]() mutable {
                    clients.emplace(std::make_pair(n, std::move(client)));
                    seastar::print("Imported remote actor interface\n");
                }).then([this, n] {
                    return raise(n);
                });
            });
        };

        template<typename ActorKey, typename Ret, typename Class, typename ...Args>
        constexpr auto
        remote_dispatch(node const &n, ActorKey const &key, Ret (Class::*fptr)(Args...) const, uint32_t id) {
            return proto.make_client<Ret(ActorKey, Args...)>(id)(*clients[n], key);
        }

        template<typename ActorKey, typename Ret, typename Class, typename ...FArgs, typename ...Args>
        constexpr auto
        remote_dispatch(node const &n, ActorKey const &key, Ret (Class::*fptr)(FArgs...) const, uint32_t id,
                        Args &&... args) {
            return proto.make_client<Ret(ActorKey, FArgs...)>(id)(*clients[n], key, std::forward<Args>(args) ...);
        }

        template<typename ActorKey, typename Ret, typename Class, typename ...Args>
        constexpr auto remote_dispatch(node const &n, ActorKey const &key, Ret (Class::*fptr)(Args...), uint32_t id) {
            return proto.make_client<Ret(ActorKey, Args...)>(id)(*clients[n], key);
        }

        template<typename ActorKey, typename Ret, typename Class, typename ...FArgs, typename ...Args>
        constexpr auto
        remote_dispatch(node const &n, ActorKey const &key, Ret (Class::*fptr)(FArgs...), uint32_t id,
                        Args &&... args) {
            return proto.make_client<Ret(ActorKey, FArgs...)>(id)(*clients[n], key, std::forward<Args>(args) ...);
        }

        // needed by seastar::sharded<T>
        seastar::future<> stop() {
            return seastar::do_for_each(clients, [](auto const &client) {
                return client.second->stop();
            });
        }
    };
}