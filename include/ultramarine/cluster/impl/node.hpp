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

#include <seastar/net/inet_address.hh>
#include <seastar/net/ip.hh>
#include "message_serializer.hpp"

namespace ultramarine::cluster {
    struct node {
        seastar::net::inet_address addr;
        uint16_t port;

        node() = default;
        node(seastar::net::inet_address const& addr, uint16_t port) : addr(addr), port(port) {}
        node(uint32_t ip4, uint16_t port) : addr(seastar::net::ipv4_address(ip4)), port(port) {}
        node(seastar::sstring ip4, uint16_t port) : addr(seastar::net::ipv4_address(ip4)), port(port) {}
        node(uint16_t port) : addr(seastar::net::ipv4_address("127.0.0.1")), port(port) {} // FIXME

        node(node const&) = default;
        node(node &&) = default;

        node & operator=(node const& n) = default;
        node & operator=(node && n) = default;

        bool operator==(const node &rhs) const {
            return addr == rhs.addr && port == rhs.port;
        }

        bool operator!=(const node &rhs) const {
            return !(rhs == *this);
        }

        std::pair<uint32_t, uint16_t> make_identity() const {
            return std::make_pair(addr.as_ipv4_address().ip.raw, port);
        }

        char *to_string() const {
            return inet_ntoa(::in_addr(addr));
        }

        template <typename Serializer, typename Output>
        inline void serialize(Serializer, Output& out) const {
            write_arithmetic_type(out, uint32_t(addr.as_ipv4_address().ip.raw));
            write_arithmetic_type(out, uint16_t (port));
        }

        template <typename Serializer, typename Input>
        static inline node deserialize(Serializer, Input& in) {
            auto addr = read_arithmetic_type<uint32_t>(in);
            auto port = read_arithmetic_type<uint16_t >(in);
            return node(addr, port);
        }
    };
}

namespace std {
    template<>
    struct hash<ultramarine::cluster::node> {
        size_t operator()(const ultramarine::cluster::node& node) const {
            std::size_t seed = 0;
            boost::hash_combine(seed, std::hash<seastar::net::inet_address>{}(node.addr));
            boost::hash_combine(seed, node.port);
            return seed;
        }
    };
}