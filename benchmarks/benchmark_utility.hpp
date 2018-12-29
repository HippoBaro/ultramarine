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

#include <chrono>
#include <numeric>
#include <seastar/core/app-template.hh>
#include <ultramarine/silo.hpp>

#define ULTRAMARINE_BENCH(name) {#name, name}

namespace ultramarine::benchmark {

    using benchmark_list = std::initializer_list<std::pair<std::string_view, seastar::future<> (*)()>>;

    template<typename Pair>
    auto run_one(Pair &&bench, int run) {
        int *counter = new int(0);
        return seastar::do_with(std::move(bench),
                                std::vector<std::chrono::microseconds::rep>(),
                                [counter, run](auto &bench, auto &vec) {
                                    return seastar::do_until([counter, run] {
                                        return *counter >= run;
                                    }, [counter, &bench, &vec] {
                                        ++*counter;
                                        using namespace std::chrono;
                                        auto start = high_resolution_clock::now();
                                        return std::get<1>(bench)().then([start, &vec] {
                                            auto stop = high_resolution_clock::now();
                                            vec.emplace_back(duration_cast<microseconds>(stop - start).count());
                                            return seastar::make_ready_future();
                                        });
                                    }).then([&vec, &bench] {
                                        std::sort(std::begin(vec), std::end(vec));
                                        auto sum = std::accumulate(std::begin(vec), std::end(vec), 0);
                                        seastar::print("%s: %lu us (min: %lu us -- 99.9p: %lu us)\n", std::get<0>(bench),
                                                       sum / (vec.size()), *std::begin(vec), *(std::end(vec) - 1));
                                    });
                                });
    }

    int run(int ac, char **av, benchmark_list &&benchs, int run = 1000) {
        seastar::app_template app;

        return app.run(ac, av, [benchs = std::move(benchs), run] {
            return seastar::do_with(std::move(benchs),
                                    std::vector<std::chrono::microseconds::rep>(),
                                    [run](auto &benchs, auto &vec) {
                                        auto silo = new ultramarine::silo_server();
                                        return silo->start().then([silo, run, &benchs] {
                                            seastar::engine().at_exit([silo] {
                                                return silo->stop().then([silo] {
                                                    delete silo;
                                                    return seastar::make_ready_future();
                                                });
                                            });
                                            return seastar::do_for_each(benchs, [run](auto &bench) {
                                                return run_one(bench, run);
                                            });
                                        });
                                    });
        });
    }
}