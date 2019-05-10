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

#include <seastar/core/future.hh>
#include <seastar/core/reactor.hh>
#include <ultramarine/cluster/impl/server.hpp>
#include <ultramarine/cluster/impl/membership.hpp>

namespace ultramarine::cluster {
    template<typename Func>
    auto with_cluster(seastar::socket_address const &local, std::vector<seastar::socket_address> &&peers, Func &&func) {
        return seastar::when_all_succeed(impl::membership::service.start(local),
                                         impl::server::service.start(local)).then([peers = std::move(peers)] {
            seastar::engine().at_exit([] {
                return seastar::when_all(impl::membership::service.stop(),
                                         impl::server::service.stop()).discard_result();
            });
            return seastar::parallel_for_each(peers, [](seastar::socket_address const &peer) {
                return impl::membership::service.invoke_on_all([peer](auto &service) {
                    return service.try_add_peer(peer);
                });
            }).then([peers] {
                if (!peers.empty() && !impl::membership::service.local().is_connected_to_cluster()) {
                    return seastar::make_exception_future(std::runtime_error("Failed to join cluster"));
                } else if (peers.empty()) {
                    seastar::print("No cluster to join, assuming bootstrap node\n");
                }
                return seastar::make_ready_future();
            });
        }).discard_result().then([func = std::forward<Func>(func)] {
            seastar::print("Calling user code\n");
            return func();
        });
    }
}