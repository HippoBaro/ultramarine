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
#include <ultramarine/message_deduplicate.hpp>
#include "benchmark_utility.hpp"

static constexpr std::size_t ProduceCount = 20000000;

class counting_actor : public ultramarine::actor<counting_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(counting_actor, (count)(increment));
    std::size_t discovered = 0;

    std::size_t count() const {
        return discovered;
    }

    void increment() {
        ++discovered;
    }
};

class producer_actor : public ultramarine::actor<producer_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(producer_actor, (produce));
    std::size_t produced;

    seastar::future<> produce(int counter_addr) {
        auto counter = ultramarine::get<counting_actor>(counter_addr);

        return ultramarine::deduplicate(counter, counting_actor::message::increment(), [this] (auto &increment) {
            for (produced = 0; produced < ProduceCount; ++produced) {
                increment();
            }
        }).then([this, counter] {
            return counter->count().then([this](std::size_t discovered) {
                assert(produced == discovered);
            });
        });
    }
};

seastar::future<> count_collocated() {
    return producer_actor::clear_directory().then([] {
        return counting_actor::clear_directory().then([] {
            return ultramarine::get<producer_actor>(0)->produce(1);
        });
    });
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(count_collocated),
    }, 100);
}