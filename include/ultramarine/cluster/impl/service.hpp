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

#include <seastar/core/sharded.hh>
#include "endpoint_service.hpp"
#include "distributed_directory.hpp"

/// \exclude
namespace ultramarine::cluster {
    /// \exclude
    class cluster_service {
    private:
        seastar::sharded<exported_actor_endpoints_service> export_service;
        seastar::sharded<imported_actor_endpoints_service> import_service;
        seastar::sharded<distributed_directory> directory_service;
    public:

        seastar::future<> init(seastar::sstring node_name, seastar::ipv4_addr const& bind_to) {
            return directory_service.start(std::move(node_name)).then([this, bind_to] {
                return seastar::when_all_succeed(export_service.start(bind_to), import_service.start()).discard_result();
            });
        }

        seastar::future<> connect(seastar::sstring nodename, seastar::socket_address addr) {
            return import_service.invoke_on_all(&imported_actor_endpoints_service::connect, addr).then(
                    [this, nodename = std::move(nodename)] {
                        return directory_service.invoke_on_all(&distributed_directory::add_remote_directory, nodename);
                    });
        }

        inline distributed_directory& directory() {
            if (!directory_service.local_is_initialized()) {
                seastar::log("panic: accessing uninitialized cluster directory\n");
                std::abort();
            }
            return directory_service.local();
        }

        inline imported_actor_endpoints_service& import() {
            return import_service.local();
        }

        seastar::future<> stop() {
            return seastar::when_all_succeed(export_service.stop(), import_service.stop(),
                                             directory_service.stop()).discard_result();
        }
    };

    static cluster_service &service() {
        static cluster_service s = cluster_service{};
        return s;
    }
}