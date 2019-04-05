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
#include <seastar/core/execution_stage.hh>

static constexpr std::size_t CreationCount = 16000000;

class create_actor : public ultramarine::actor<create_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(create_actor, (process));
    void process() {
        auto volatile sint = std::sin(37.2);
        auto volatile res = sint * sint;
        //defeat dead code elimination
        assert(res > 0);
    }

};

auto inline make(int i) {

}

static int i; // need to be not on stack
seastar::future<> fork_join_create_naive() {
    i = 0;
    return create_actor::clear_directory().then([] {
        return seastar::do_until([] { return i >= CreationCount; }, [] {
            return ultramarine::get<create_actor>(i++).tell(create_actor::message::process());
        });
    });
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(fork_join_create_naive)
    }, 10);
}