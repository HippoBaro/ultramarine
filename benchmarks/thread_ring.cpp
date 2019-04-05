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
#include <seastar/core/execution_stage.hh>
#include "benchmark_utility.hpp"

static constexpr std::size_t RingSize = 1000000;
static constexpr std::size_t MessageCount = 1000000;

class thread_ring_actor : public ultramarine::actor<thread_ring_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(thread_ring_actor, (ping));
    ultramarine::actor_id const next = (key + 1) % RingSize;
    seastar::future<> ping(int remaining) {
        if (remaining > 1) {
            return ultramarine::get<thread_ring_actor>(next).tell(thread_ring_actor::message::ping(), remaining - 1);
        }
        return seastar::make_ready_future<>();
    }
};

seastar::future<> thread_ring() {
    return thread_ring_actor::clear_directory().then([] {
        return ultramarine::get<thread_ring_actor>(0).tell(thread_ring_actor::message::ping(), MessageCount);
    });
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(thread_ring)
    }, 10);
}