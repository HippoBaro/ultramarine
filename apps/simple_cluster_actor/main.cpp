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

#include <chrono>
#include <boost/range.hpp>
#include <seastar/core/app-template.hh>
#include <ultramarine/actor_ref.hpp>
#include <seastar/core/sleep.hh>
#include <seastar/core/print.hh>
#include <ultramarine/cluster/cluster.hpp>
#include <ultramarine/utility.hpp>
#include "simple_actor.hpp"

std::chrono::time_point<std::chrono::system_clock> start;
std::chrono::time_point<std::chrono::system_clock> end;

auto range = boost::irange(0, 1000000);

//int main(int ac, char **av) {
//    seastar::app_template app;
//
//    fmt::print("actor_ref size: {}\n", sizeof(ultramarine::actor_ref<simple_actor>));
//    fmt::print(" -- local_actor_ref size: {}\n", sizeof(ultramarine::impl::collocated_actor_ref<simple_actor>));
//    fmt::print(" -- remote_actor_ref size: {}\n", sizeof(ultramarine::cluster::impl::remote_actor_ref<simple_actor>));
//
//    fmt::print("actor size: {}\n", sizeof(simple_actor));
//    fmt::print(" -- key size: {}\n", sizeof(simple_actor::KeyType));
//
//    return app.run(ac, av, [] {
//        std::vector<seastar::socket_address> peers{
//                seastar::socket_address(seastar::net::inet_address("127.0.0.1"), 5555U)
//        };
//
//        return ultramarine::cluster::with_cluster(
//                seastar::socket_address(seastar::net::inet_address("127.0.0.1"), 5556U), std::move(peers), []() {
//                    seastar::print("In user code\n");
//                    start = std::chrono::system_clock::now();
//                    return ultramarine::with_buffer(10000, [](auto &buffer) {
//                        return seastar::do_for_each(range.begin(), range.end(), [&buffer](int i) {
//                            return buffer(ultramarine::get<simple_actor>(0)->say_hello());
//                        });
//                    }).then([] {
//                        using namespace std::chrono;
//                        end = std::chrono::system_clock::now();
//                        auto span = end - start;
//                        fmt::print("{} mes/sec\n", (float) (*range.end()) * 2 /
//                                                   ((float) duration_cast<milliseconds>(span).count() / 1000));
//                    });
//                }).then([] {
//            seastar::print("Out of user code\n");
//        });
//    });
//}


int main(int ac, char **av) {
    seastar::app_template app;

    fmt::print("actor_ref size: {}\n", sizeof(ultramarine::actor_ref<simple_actor>));
    fmt::print(" -- local_actor_ref size: {}\n", sizeof(ultramarine::impl::collocated_actor_ref<simple_actor>));
    fmt::print(" -- remote_actor_ref size: {}\n", sizeof(ultramarine::cluster::impl::remote_actor_ref<simple_actor>));

    fmt::print("actor size: {}\n", sizeof(simple_actor));
    fmt::print(" -- key size: {}\n", sizeof(simple_actor::KeyType));

    return app.run(ac, av, [] {
        return ultramarine::cluster::with_cluster(seastar::socket_address(seastar::net::inet_address("127.0.0.1"), 5555U), { }, [] {
            return seastar::sleep(std::chrono::hours(10));
        });
    });
}