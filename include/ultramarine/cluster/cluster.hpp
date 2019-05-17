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
    seastar::future<>
    with_cluster_impl(seastar::socket_address const &local, std::vector<seastar::socket_address> &&peers);

    template<typename Func>
    seastar::future<>
    with_cluster(seastar::socket_address const &local, std::vector<seastar::socket_address> &&peers,
                 std::size_t minimum_connected_peers, Func &&func) {
        return with_cluster_impl(local, std::move(peers)).then(
                [func = std::forward<Func>(func), minimum_connected_peers]() mutable {
                    return impl::membership::service.local().joined_cv.wait([minimum_connected_peers] {
                        return impl::membership::service.local().members().size() >= minimum_connected_peers;
                    }).then([func = std::forward<Func>(func)] {
                        seastar::print("%d: Calling user code\n", seastar::engine().cpu_id());
                        return func();
                    });
                });
    }

    template<typename Func>
    seastar::future<>
    with_cluster(seastar::socket_address const &local, std::vector<seastar::socket_address> &&peers, Func &&func) {
        return with_cluster(local, std::move(peers), 0, std::forward<Func>(func));
    }

    template<typename Func>
    seastar::future<>
    with_cluster(seastar::socket_address const &local, std::size_t minimum_connected_peers, Func &&func) {
        return with_cluster(local, {}, minimum_connected_peers, std::forward<Func>(func));
    }

    template<typename Func>
    seastar::future<>
    with_cluster(seastar::socket_address const &local, Func &&func) {
        return with_cluster(local, {}, 0, std::forward<Func>(func));
    }
}