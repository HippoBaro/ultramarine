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

#include <ultramarine/cluster/impl/membership.hpp>
#include "ultramarine/cluster/impl/server.hpp"

namespace ultramarine::cluster::impl {
    static inline std::pair<char *, std::size_t> make_peer_string_identity(seastar::socket_address const &endpoint) {
        static thread_local char identity[21];

        auto ip = endpoint.addr().as_ipv4_address().ip.raw;
        auto res = fmt::format_to_n(identity, sizeof(identity), "{}.{}.{}.{}:{}", (ip >> 24U) & 0xFFU,
                                    (ip >> 16U) & 0xFFU, (ip >> 8U) & 0xFFU, ip & 0xFFU, endpoint.port());
        *res.out = '\0';
        return {identity, res.size};
    }

    server::server(seastar::socket_address const &local) : local(local) {
        for (const auto &handler : message_handler_registry()) {
            handler.second(&proto);
        }
        proto.register_handler(0, [this](handshake_request req) {
            auto id = make_peer_string_identity(req.origin);
            seastar::print("%u: Received handshake from %s\n", seastar::engine().cpu_id(), id.first);
            return membership::service.invoke_on_all([req = std::move(req)](membership &service) mutable {
                return service.add_candidates(std::move(req));
            }).then([] {
                std::vector<seastar::socket_address> vec;
                for (const auto &member : membership::service.local().members()) {
                    vec.emplace_back(member.second);
                }
                return handshake_response(std::move(vec));
            });
        });
        rpc = std::make_unique<rpc_proto::server>(proto, local);
    }

    seastar::future<> server::stop() {
        return rpc->stop();
    }

    seastar::future<> server::start(seastar::socket_address const& local) {
        return service.start(local);
    }
}


