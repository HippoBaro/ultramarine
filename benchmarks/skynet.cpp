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

#include <ultramarine/actor.hpp>
#include <ultramarine/actor_ref.hpp>
#include "benchmark_utility.hpp"

class skynet_singleton_actor : public ultramarine::actor<skynet_singleton_actor> {
public:
    using KeyType = unsigned long;
ULTRAMARINE_DEFINE_ACTOR(skynet_singleton_actor, (skynet));

    seastar::future<unsigned long> skynet(unsigned long num, unsigned long size, unsigned int div) const {
        if (size == 1) {
            return seastar::make_ready_future<unsigned long>(num);
        }

        std::vector<seastar::future<unsigned long>> tasks;
        tasks.reserve(div);

        for (int i = 0; i < div; ++i) {
            auto sub_num = num + i * (size / div);
            tasks.emplace_back(
                    ultramarine::get<skynet_singleton_actor>(sub_num).tell(skynet_singleton_actor::message::skynet(),
                                                                           (unsigned long) sub_num,
                                                                           (unsigned long) size / div,
                                                                           (unsigned int) div));
        }

        return seastar::do_with(std::move(tasks), [](auto &tasks) {
            return seastar::when_all(std::begin(tasks), std::end(tasks)).then([](auto tasks) {
                return seastar::map_reduce(std::begin(tasks), std::end(tasks), [](auto &result) {
                    return std::move(result);
                }, 0UL, std::plus<unsigned long>());
            });
        });
    }
};

class skynet_local_actor
        : public ultramarine::actor<skynet_local_actor>, public ultramarine::local_actor<skynet_local_actor> {
public:
    using KeyType = unsigned long;
ULTRAMARINE_DEFINE_ACTOR(skynet_local_actor, (skynet));

    seastar::future<unsigned long> skynet(unsigned long num, unsigned long size, unsigned int div) const {
        if (size == 1) {
            return seastar::make_ready_future<unsigned long>(num);
        }

        std::vector<seastar::future<unsigned long>> tasks;
        tasks.reserve(div);

        for (int i = 0; i < div; ++i) {
            auto sub_num = num + i * (size / div);
            tasks.emplace_back(ultramarine::get<skynet_local_actor>(0).tell(skynet_local_actor::message::skynet(),
                                                                            (unsigned long) sub_num,
                                                                            (unsigned long) size / div,
                                                                            (unsigned int) div));
        }

        return seastar::do_with(std::move(tasks), [](auto &tasks) {
            return seastar::when_all(std::begin(tasks), std::end(tasks)).then([](auto tasks) {
                return seastar::map_reduce(std::begin(tasks), std::end(tasks), [](auto &result) {
                    return std::move(result);
                }, 0UL, std::plus{});
            });
        });
    }
};

static seastar::future<unsigned long> skynet(unsigned long num, unsigned long size, unsigned int div) {
    if (size == 1) {
        return seastar::make_ready_future<unsigned long>(num);
    }

    std::vector<seastar::future<unsigned long>> tasks;
    tasks.reserve(div);

    for (int i = 0; i < div; ++i) {
        auto sub_num = num + i * (size / div);
        tasks.emplace_back(skynet(sub_num, size / div, div));
    }

    return seastar::do_with(std::move(tasks), [](auto &tasks) {
        return seastar::when_all(std::begin(tasks), std::end(tasks)).then([](auto tasks) {
            return seastar::map_reduce(std::begin(tasks), std::end(tasks), [](auto &result) {
                return std::move(result);
            }, 0UL, std::plus{});
        });
    });
};

constexpr auto breath = 10;
constexpr auto max = 1000000;

auto skynet_futures() {
    return skynet(0, max, breath).discard_result();
}

auto skynet_s_actor() {
    return ultramarine::get<skynet_singleton_actor>(0).tell(skynet_singleton_actor::message::skynet(), 0, max,
                                                            breath).discard_result();
}

auto skynet_l_actor() {
    return ultramarine::get<skynet_local_actor>(0).tell(skynet_local_actor::message::skynet(), 0, max,
                                                        breath).discard_result();
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(skynet_futures),
            ULTRAMARINE_BENCH(skynet_s_actor),
            ULTRAMARINE_BENCH(skynet_l_actor),
    }, 10);
}