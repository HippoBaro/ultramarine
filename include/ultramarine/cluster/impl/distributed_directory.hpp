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
#include <string_view>
#include <seastar/core/future.hh>
#include "node.hpp"
#include "distributed_membership_service.hpp"

extern "C" {
#include <hash_ring.h>
}

namespace ultramarine::cluster {
    /// \exclude
    struct distributed_directory : public seastar::weakly_referencable<distributed_directory> {
    private:
        using ring_ptr = std::unique_ptr<hash_ring_t, void (*)(hash_ring_t *)>;
        ring_ptr ring;
        std::unique_ptr<event_listener<node>> imported_endpoint_listener = nullptr;
        node local_node;

    public:
        explicit distributed_directory(node const &local,
                                       seastar::sharded<imported_actor_endpoints_service> *endpoint) :
                ring(ring_ptr(hash_ring_create(1, HASH_FUNCTION_SHA1), hash_ring_free)), local_node(local) {
            auto identity = local_node.make_identity();
            hash_ring_add_node(ring.get(), (uint8_t *) &identity, sizeof(identity));
        }

        void set_event_listener(std::unique_ptr<event_listener<node>> listener) {
            imported_endpoint_listener = std::move(listener);
            imported_endpoint_listener->set_callback([this](node const &n) {
                auto identity = n.make_identity();
                hash_ring_add_node(ring.get(), (uint8_t *) &identity, sizeof(identity));
                return seastar::make_ready_future();
            });
        };

        std::optional<node> node_for_key(std::size_t key) const {
            auto *ptr = hash_ring_find_node(ring.get(), (uint8_t *) &key, sizeof(key));
            auto pair = (std::pair<uint32_t, uint16_t> *) ptr->name;
            auto n = node(pair->first, pair->second);
            if (n != local_node) {
                return n;
            }
            return {};
        }

        // needed by seastar::sharded<T>
        seastar::future<> stop() {
            return seastar::make_ready_future();
        }
    };
}