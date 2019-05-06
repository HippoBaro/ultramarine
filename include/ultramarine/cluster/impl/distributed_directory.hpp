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

extern "C" {
#include <hash_ring.h>
}

namespace ultramarine::cluster {
    /// \exclude
    struct distributed_directory {
    private:
        using ring_ptr = std::unique_ptr<hash_ring_t, void (*)(hash_ring_t *)>;
        ring_ptr ring;
        std::string node_name;

    public:
        explicit distributed_directory(seastar::sstring nodename) :
                ring(ring_ptr(hash_ring_create(1, HASH_FUNCTION_SHA1), hash_ring_free)),
                node_name(nodename.c_str(), nodename.size()) {
            hash_ring_add_node(ring.get(), (uint8_t *)node_name.c_str(), node_name.size());
        }

        void add_remote_directory(seastar::sstring nodename) {
            hash_ring_add_node(ring.get(), (uint8_t *)nodename.c_str(), nodename.size());
        }

        void remove_remote_directory(seastar::sstring nodename) {
            hash_ring_remove_node(ring.get(), (uint8_t *)nodename.c_str(), nodename.size());
        }

        auto node_for_key(std::size_t key) {
            auto *n = hash_ring_find_node(ring.get(), (uint8_t *)&key, sizeof(key));
            assert(n != nullptr);

            return node(std::string_view((const char *) n->name, n->nameLen) == node_name);
        }

        // needed by seastar::sharded<T>
        seastar::future<> stop() {
            return seastar::make_ready_future();
        }
    };
}