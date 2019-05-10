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

#include <utility>
#include <seastar/core/future-util.hh>
#include "ultramarine/cluster/impl/membership.hpp"
#include "ultramarine/cluster/impl/message_handler_registry.hpp"

namespace ultramarine::cluster::impl {

    static inline std::pair<char *, std::size_t> make_peer_string_identity(seastar::socket_address const &endpoint) {
        static thread_local char identity[21];

        auto ip = endpoint.addr().as_ipv4_address().ip.raw;
        auto res = fmt::format_to_n(identity, sizeof(identity), "{}.{}.{}.{}:{}", (ip >> 24U) & 0xFFU,
                                    (ip >> 16U) & 0xFFU, (ip >> 8U) & 0xFFU, ip & 0xFFU, endpoint.port());
        *res.out = '\0';
        return {identity, res.size};
    }

    static inline std::pair<uint32_t, uint16_t> make_peer_identity(seastar::socket_address const &endpoint) {
        return std::make_pair(endpoint.addr().as_ipv4_address().ip.raw, endpoint.port());
    }

    membership::membership(seastar::socket_address const &local) :
            candidates(100), candidate_connection_job(seastar::make_ready_future()),
            ring(ring_ptr(hash_ring_create(1, HASH_FUNCTION_SHA1), hash_ring_free)), local_node(local) {
        for (const auto &handler : message_handler_registry()) {
            handler.second(&proto);
        }

        proto.set_logger([](seastar::sstring log) {
            seastar::print("%u: RPC -> %s\n", seastar::engine().cpu_id(), log);
        });

        auto identity = make_peer_identity(local);
        hash_ring_add_node(ring.get(), (uint8_t *) &identity, sizeof(identity));
        candidate_connection_job = contact_candidates();
    }

    seastar::future<> membership::try_add_peer(seastar::socket_address endpoint) {
        auto id = make_peer_string_identity(endpoint);
        // If the endpoint is already part of the cluster;
        if (nodes.count(node(endpoint.addr().as_ipv4_address().ip.raw, endpoint.port())) > 0) {
            seastar::print("%u: Peer %s is already part of the cluster, skipping\n", seastar::engine().cpu_id(),
                           id.first);
            return seastar::make_ready_future();
        }
        // If the endpoint is already being contacted
        if (connecting_nodes.count(endpoint) > 0) {
            seastar::print("%u: Peer %s is already being contacted, skipping\n", seastar::engine().cpu_id(), id.first);
            return seastar::make_ready_future();
        }
        connecting_nodes.insert(endpoint);
        return connect(endpoint).then([this](seastar::lw_shared_ptr<rpc_proto::client> client) {
            return seastar::do_with(std::move(client), [this](seastar::lw_shared_ptr<rpc_proto::client> &client) {
                return handshake(client).then([this, &client](handshake_response response) {
                    auto id = make_peer_string_identity(client->peer_address());
                    auto identity = make_peer_identity(client->peer_address());
                    nodes.emplace(node(&proto, std::move(client)));
                    hash_ring_add_node(ring.get(), (uint8_t *) &identity, sizeof(identity));
                    seastar::print("%u: Added peer %s to hash-ring\n", seastar::engine().cpu_id(), id.first);
                    return seastar::make_ready_future();
                });
            });
        }).handle_exception([endpoint](auto ex) {
            auto identity = make_peer_string_identity(endpoint);
            seastar::print("%u: Failed to contact peer at %s\n", seastar::engine().cpu_id(), identity.first);
        }).finally([this, endpoint] {
            connecting_nodes.erase(endpoint);
        });
    }

    node const *membership::node_for_key(std::size_t key) const {
        auto *ptr = hash_ring_find_node(ring.get(), (uint8_t *) &key, sizeof(key));
        auto pair = (std::pair<uint32_t, uint16_t> *) ptr->name;
        if (*pair != make_peer_identity(local_node)) {
            return &(*nodes.find(node(pair->first, pair->second)));
        }
        return nullptr;
    }

    seastar::future<> membership::stop() {
        seastar::print("closing gate\n");
        return candidate_connection_gate.close().then([this] {
            return seastar::do_until(
                    [this] { return candidate_connection_job.available() || candidate_connection_job.failed(); },
                    [this] {
                        candidates.abort(std::make_exception_ptr(std::exception()));
                        return seastar::make_ready_future();
                    })
                    .then([this] {
                        return seastar::parallel_for_each(nodes, [this](auto &peer) {
                            return disconnect(peer);
                        });
                    });
        });
    }

    seastar::future<> membership::start(seastar::socket_address const &local) {
        return service.start(local);
    }

    bool membership::is_connected_to_cluster() const {
        return !nodes.empty();
    }

    std::unordered_set<node> const &membership::members() const {
        return nodes;
    }

    seastar::future<seastar::lw_shared_ptr<rpc_proto::client>> membership::connect(seastar::socket_address const &to) {
        return seastar::do_with(seastar::make_lw_shared<rpc_proto::client>(proto, to), [](auto &client) {
            return client->await_connection().then([&client] {
                auto identity = make_peer_string_identity(client->peer_address());
                seastar::print("%u: Connected to %s\n", seastar::engine().cpu_id(), identity.first);
                return client;
            });
        });
    }

    seastar::future<handshake_response>
    membership::handshake(seastar::lw_shared_ptr<rpc_proto::client> &with) {
        auto identity = make_peer_string_identity(with->peer_address());
        seastar::print("%u: Performing handshake with %s\n", seastar::engine().cpu_id(), identity.first);
        auto hs = proto.make_client<handshake_response(handshake_request)>(0);
        return hs(*with,
                  handshake_request(std::vector<seastar::socket_address>(nodes.begin(), nodes.end()), local_node));
    }

    seastar::future<> membership::disconnect(node const &n) const {
        return n.client->stop().then_wrapped([](seastar::future<>) {
            return seastar::make_ready_future();
        });
    }

    seastar::future<> membership::contact_candidates() {
        return seastar::repeat([this] {
            return candidates.pop_eventually().then([this](seastar::socket_address addr) {
                return seastar::with_gate(candidate_connection_gate, [this, addr] {
                    return candidates.pop_eventually().then([this](seastar::socket_address addr) {
                        return try_add_peer(addr).then([] {
                            return seastar::stop_iteration::no;
                        });
                    });
                });
            }).handle_exception([](std::exception_ptr ex) {
                return seastar::stop_iteration::yes;
            }); // gate closed
        });
    }

    seastar::future<> membership::add_candidates(handshake_request req) {
        auto add = [this](seastar::socket_address addr) {
            return candidates.push_eventually(std::move(addr));
        };
        return seastar::do_with(std::move(req), [add](handshake_request &req) {
            return add(req.origin).then([add, &req] {
                return seastar::do_for_each(req.known_nodes, [add](seastar::socket_address &addr) {
                    return add(addr);
                });
            });
        });
    }
}





