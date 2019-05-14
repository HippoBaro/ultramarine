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
#include <ultramarine/message_deduplicate.hpp>
#include "benchmark_utility.hpp"

static constexpr std::size_t Iteration = 10000;
static constexpr std::size_t WorkerCount = 1000;

class throughput_actor : public ultramarine::actor<throughput_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(throughput_actor, (process)(create));

    void process() {
        auto volatile sint = std::sin(37.2);
        auto volatile res = sint * sint;
        //defeat dead code elimination
        assert(res > 0);
    }

    void create() const {}

};

static int i; // need to be not on stack
seastar::future<> fork_join_throughput() {
    return seastar::parallel_for_each(boost::irange(0UL, WorkerCount), [](int i) {
        return ultramarine::deduplicate(ultramarine::get<throughput_actor>(i), throughput_actor::message::process(), []
                (auto &d) {
            for (int j = 0; j < Iteration; ++j) {
                d();
            }
        });
    });
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(fork_join_throughput),
    }, 10);
}