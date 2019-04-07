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
#include "benchmark_utility.hpp"

static constexpr std::size_t PingPongCount = 1000;
static constexpr std::size_t ActorCount = 1000;

class pseudo_random {
    static thread_local long value;

public:
    static long nextLong() {
        return value = ((value * 1309) + 13849) & 65535;
    }

    static int nextInt() {
        return static_cast<int>(nextLong());
    }

    static double nextDouble() {
        return 1.0 / (nextLong() + 1);
    }

    static int nextInt(int exclusive_max) {
        return nextInt() % exclusive_max;
    }
};

thread_local long pseudo_random::value = 74755;

class big_actor : public ultramarine::actor<big_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(big_actor, (ping)(pong));
    std::size_t pingpong_count = 0;

    seastar::future<> ping() {
        return ultramarine::with_buffer(100, [this] (auto &buffer) {
            return seastar::do_until([this] { return pingpong_count >= PingPongCount; }, [this, &buffer] {
                ++pingpong_count;
                auto next = pseudo_random::nextInt(ActorCount);
                return buffer(ultramarine::get<big_actor>(next).tell(big_actor::message::pong()));
            });
        });
    };

    void pong() const { };
};

int i;
seastar::future<> big() {
    i = 0;
    return big_actor::clear_directory().then([] {
        return ultramarine::with_buffer(100, [] (auto &buffer) {
            return seastar::do_until([] { return i >= ActorCount; }, [&buffer] {
                return buffer(ultramarine::get<big_actor>(i++).tell(big_actor::message::ping()));
            });
        });
    });
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(big)
    }, 10);

}