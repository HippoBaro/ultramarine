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

#include <ultramarine/cluster/cluster.hpp>
#include <seastar/core/future-util.hh>
#include <seastar/core/sleep.hh>

namespace ultramarine::cluster {
    static seastar::future<>
    try_join(seastar::socket_address const &local, std::vector<seastar::socket_address> const &peers) {
        return impl::server::service.start(local).then([&local, &peers] {
            return impl::membership::service.start(local).then([&peers] {
                return seastar::parallel_for_each(peers, [](seastar::socket_address const &peer) {
                    return impl::membership::service.invoke_on_all([peer](auto &service) {
                        return service.try_add_peer(peer);
                    });
                }).then([&peers] {
                    if (!peers.empty() && !impl::membership::service.local().is_connected_to_cluster()) {
                        return seastar::when_all(impl::membership::service.stop(),
                                                 impl::server::service.stop()).then([](auto) {
                            return seastar::make_exception_future(std::runtime_error("Failed to join cluster"));
                        });

                    } else if (peers.empty()) {
                        seastar::print("%d: No cluster to join, assuming bootstrap node\n", seastar::engine().cpu_id());
                    }
                    return seastar::make_ready_future();
                });
            });
        });
    };

    seastar::future<>
    with_cluster_impl(seastar::socket_address const &local, std::vector<seastar::socket_address> &&peers) {
        return do_with(int{0}, local, std::move(peers), []
                (int &i, seastar::socket_address const &local, std::vector<seastar::socket_address> const &peers) {
            return seastar::repeat([&i, &local, &peers] {
                return try_join(local, peers).then([] {
                    return seastar::stop_iteration::yes;
                }).then_wrapped([&i](auto&& fut) {
                    if (fut.failed()) {
                        try {
                            std::rethrow_exception(fut.get_exception());
                        }
                        catch (std::system_error const &) {
                            throw;
                        }
                        catch (std::runtime_error const &ex) {
                            ++i;
                            seastar::print("%d: Failed to join cluster (%d/5 try)\n", seastar::engine().cpu_id(), i);
                            if (i >= 5) {
                                throw ex;
                            }
                            int backoff = std::pow<int>(2, i);
                            seastar::print("%d: Retrying in %d seconds...\n", seastar::engine().cpu_id(), backoff);
                            return seastar::sleep(std::chrono::seconds(backoff)).then([] {
                                return seastar::stop_iteration::no;
                            });
                        }
                    }
                    return seastar::make_ready_future<seastar::bool_class<seastar::stop_iteration_tag>>(
                            seastar::stop_iteration::yes);
                });
            }).then([] {
                seastar::engine().at_exit([] {
                    return seastar::when_all(impl::membership::service.stop(),
                                             impl::server::service.stop()).discard_result();
                });
            }).handle_exception([](std::exception_ptr ex) {
                return seastar::make_exception_future(ex);
            });
        });
    }
}
