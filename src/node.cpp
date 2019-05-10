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

#include <ultramarine/cluster/impl/node.hpp>

namespace ultramarine::cluster::impl {
    node::node(uint32_t ip4, uint16_t port) : endpoint(seastar::net::ipv4_address(ip4), port), client(nullptr),
                                              rpc(nullptr) {}

    node::node(rpc_proto *proto, seastar::lw_shared_ptr<rpc_proto::client> &&client) : endpoint(client->peer_address()),
                                                                                       client(std::move(client)),
                                                                                       rpc(proto) {}

    bool node::operator==(const node &rhs) const {
        return endpoint == rhs.endpoint;
    }

    bool node::operator!=(const node &rhs) const {
        return !(rhs == *this);
    }

    node::operator seastar::socket_address() const {
        return endpoint;
    }
}

namespace std {
    size_t hash<ultramarine::cluster::impl::node>::operator()(ultramarine::cluster::impl::node const &node) const {
        std::size_t seed = 0;
        boost::hash_combine(seed, std::hash<uint32_t>{}(node.endpoint.addr().as_ipv4_address().ip.raw));
        boost::hash_combine(seed, node.endpoint.port());
        return seed;
    }
}
