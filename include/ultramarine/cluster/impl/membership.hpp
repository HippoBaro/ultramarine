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

#include <memory>
#include <seastar/core/weak_ptr.hh>
#include "node.hpp"
#include "handshake.hpp"

extern "C" {
#include <hash_ring.h>
}

namespace ultramarine::cluster::impl {
    class membership : public seastar::weakly_referencable<membership> {
        using ring_ptr = std::unique_ptr<hash_ring_t, void (*)(hash_ring_t *)>;
        seastar::queue<seastar::socket_address> candidates;
        seastar::future<> candidate_connection_job;
        seastar::gate candidate_connection_gate;
        std::unordered_set<node> nodes;
        std::unordered_set<seastar::socket_address> connecting_nodes;
        ring_ptr ring;
        seastar::socket_address local_node;
        rpc_proto proto{serializer{}};

    public:
        explicit membership(seastar::socket_address const &local);

        seastar::future<> try_add_peer(seastar::socket_address endpoint);

        node const *node_for_key(std::size_t key) const;

        bool is_connected_to_cluster() const;

        std::unordered_set<node> const& members() const;

        seastar::future<> stop();

        seastar::future<> add_candidates(handshake_request req);

        seastar::future<> contact_candidates();

        static inline seastar::sharded<membership> service;

        static inline seastar::future<> start(seastar::socket_address const &local);

    private:
        seastar::future<seastar::lw_shared_ptr<rpc_proto::client>> connect(seastar::socket_address const& to);

        seastar::future<handshake_response>
        handshake(seastar::lw_shared_ptr<rpc_proto::client> &with);

        seastar::future<> disconnect(node const&n) const;
    };
}