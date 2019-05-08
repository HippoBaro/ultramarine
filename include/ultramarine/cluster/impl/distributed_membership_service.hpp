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

#include <utility>
#include <unordered_map>
#include <seastar/core/reactor.hh>
#include <seastar/core/weak_ptr.hh>
#include <seastar/core/sharded.hh>
#include "node.hpp"
#include "events.hpp"

namespace ultramarine::cluster {
    // The leader handles membership on behalf of the node
    // and communicates membership events to other shards
    class distributed_membership_service : public event_emitter<node>,
                                           public seastar::peering_sharded_service<distributed_membership_service> {
        seastar::shard_id leader;
        std::unordered_set<node> members;
        std::unique_ptr<event_listener<node>> exported_endpoint_listener = nullptr;

    public:
        distributed_membership_service(seastar::shard_id leader) : leader(leader) {}

        seastar::future<> join(std::vector<node> existing_cluster) {
            if (existing_cluster.empty()) {
                return seastar::make_ready_future();
            }
            return seastar::parallel_for_each(existing_cluster, [this](node const &n) mutable {
                return try_connect_to(n, false);
            }).then([this] {
                if (members.empty()) {
                    seastar::print("Failed to join cluster\n");
                }
            });
        }

        void set_event_listener(std::unique_ptr<event_listener<node>> listener) {
            exported_endpoint_listener = std::move(listener);
            exported_endpoint_listener->set_callback([this](node const &n) {
                if (members.count(n) == 0) {
                    return try_connect_to(n);
                }
                return seastar::make_ready_future();
            });
        };

        // needed by seastar::sharded<T>
        seastar::future<> stop() {
            return seastar::make_ready_future();
        }

    private:
        seastar::future<> try_connect_to(node const &remote, bool broadcast = true) {
            return raise(remote).then([this, remote]() {
                members.insert(remote);
                return seastar::make_ready_future();
            }).handle_exception([](auto ex) {
                return seastar::make_ready_future();
            });
        }
    };
}