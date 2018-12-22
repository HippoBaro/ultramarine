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
#include "benchmark_utility.hpp"


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
                return result.get0();
            }, 0UL, std::plus{});
        });
    });
};

auto skynet_futures() {
    return skynet(0, 1000000, 10).discard_result();
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(skynet_futures),
    }, 100);
}