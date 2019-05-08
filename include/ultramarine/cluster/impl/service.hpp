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
#include "distributed_membership_service.hpp"

/// \exclude
namespace ultramarine::cluster {
    /// \exclude
    class cluster_service {
    private:
        seastar::sharded<exported_actor_endpoints_service> export_service;
        seastar::sharded<imported_actor_endpoints_service> import_service;
        seastar::sharded<distributed_directory> directory_service;
        seastar::sharded<distributed_membership_service> membership_service;
    public:

        seastar::future<> bootstrap(node const &local, std::vector<node> &&existing_cluster) {
            return membership_service.start(seastar::engine().cpu_id()).then([this, local] {
                return directory_service.start(local, &import_service).then([this, local] {
                    return seastar::when_all_succeed(export_service.start(local),
                                                     import_service.start(local, &membership_service));
                });
            }).then([this, existing_cluster = std::move(existing_cluster)]() mutable {
                auto f1 = import_service.invoke_on_all([this](auto &service) {
                    service.set_event_listener(membership_service.local().make_listener());
                });
                auto f2 = directory_service.invoke_on_all([this](auto &service) {
                    service.set_event_listener(import_service.local().make_listener());
                });
                auto f3 = membership_service.invoke_on_all([this](auto &service) {
                    service.set_event_listener(export_service.local().make_listener());
                });
                return seastar::when_all_succeed(std::move(f1), std::move(f2)).then(
                        [this, existing_cluster = std::move(existing_cluster)]() mutable {
                            return membership_service.invoke_on_all(
                                    [existing_cluster = std::move(existing_cluster)](auto &service) mutable {
                                        return service.join(std::move(existing_cluster));
                                    });
                        });
            });
        }

        inline distributed_directory const &directory() const {
            if (!directory_service.local_is_initialized()) {
                seastar::log("panic: accessing uninitialized cluster directory\n");
                std::abort();
            }
            return directory_service.local();
        }

        inline seastar::weak_ptr<imported_actor_endpoints_service> import() {
            return import_service.local().weak_from_this();
        }

        seastar::future<> stop() {
            return seastar::when_all_succeed(export_service.stop(), import_service.stop(),
                                             directory_service.stop(), membership_service.stop()).discard_result();
        }
    };

    static cluster_service &service() {
        static cluster_service s = cluster_service{};
        return s;
    }
}