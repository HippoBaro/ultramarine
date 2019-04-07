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
#include <ultramarine/utility.hpp>
#include <seastar/core/execution_stage.hh>
#include "benchmark_utility.hpp"

static constexpr std::size_t Iteration = 10000;
static constexpr std::size_t WorkerCount = 1;

class throughput_actor : public ultramarine::actor<throughput_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(throughput_actor, (process));

    void process() {
        auto volatile sint = std::sin(37.2);
        auto volatile res = sint * sint;
        //defeat dead code elimination
        assert(res > 0);
    }

};

static int i; // need to be not on stack
seastar::future<> fork_join_throughput() {
    std::vector<std::pair<int, ultramarine::actor_ref<throughput_actor>>> actors;
    for (int j = 0; j < WorkerCount; ++j) {
        actors.emplace_back(std::make_pair(0, ultramarine::get<throughput_actor>(j)));
    }

    i = 0;
    return seastar::do_with(std::move(actors), [](auto &actors) {
        return throughput_actor::clear_directory().then([&actors] {
            return seastar::parallel_for_each(std::begin(actors), std::end(actors), [](auto &pair) {
                return std::get<1>(pair).visit([&pair](auto const &actor) {
                    return ultramarine::with_buffer(100, [&pair, &actor](auto &buffer) {
                        return seastar::do_until([&pair] {
                            return std::get<0>(pair)++ >= Iteration;
                        }, [&buffer, &actor] {
                            return buffer(actor.tell(throughput_actor::message::process()));
                        });
                    });
                });
            });
        });
    });
}

seastar::future<> fork_join_throughput_naive() {
    std::vector<ultramarine::actor_ref<throughput_actor>> actors;
    for (int j = 0; j < WorkerCount; ++j) {
        actors.emplace_back(ultramarine::get<throughput_actor>(j));
    }

    i = 0;
    return seastar::do_with(std::move(actors), [](auto const &actors) {
        return throughput_actor::clear_directory().then([&actors] {
            return seastar::do_until([] { return i >= Iteration; }, [&actors] {
                ++i;
                return seastar::parallel_for_each(std::begin(actors), std::end(actors), [](auto actor) {
                    return actor.tell(throughput_actor::message::process());
                });
            });
        });
    });

}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(fork_join_throughput),
            ULTRAMARINE_BENCH(fork_join_throughput_naive),
    }, 10);
}