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

static constexpr std::size_t ProduceCount = 20000000;

class counting_actor : public ultramarine::actor<counting_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(counting_actor, (count)(increment));
    std::size_t discovered = 0;

    std::size_t count() const;

    void increment();
};

class producer_actor : public ultramarine::actor<producer_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(producer_actor, (produce));
    std::size_t produced = 0;

    seastar::future<> produce(int counter_addr);
};

seastar::future<> producer_actor::produce(int counter_addr) {
    produced = 0;

    return ultramarine::get<counting_actor>(counter_addr).visit([this](auto const &counter) {
        return seastar::do_until([this] { return produced >= ProduceCount; }, [this, &counter] {
            ++produced;
            counter.tell(counting_actor::message::increment());
            return seastar::make_ready_future();
        }).then([this, &counter] {
            return counter.tell(counting_actor::message::count()).then([this](std::size_t discovered) {
                assert(produced == discovered);
            });
        });
    });
}

void counting_actor::increment() {
    ++discovered;
}

std::size_t counting_actor::count() const {
    return discovered;
}

seastar::future<> count_local() {
    return producer_actor::clear_directory().then([] {
        return ultramarine::get<producer_actor>(0).tell(producer_actor::message::produce(), 0);
    });
}

seastar::future<> count_collocated() {
    return producer_actor::clear_directory().then([] {
        return ultramarine::get<producer_actor>(0).tell(producer_actor::message::produce(), 1);
    });
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(count_local),
            ULTRAMARINE_BENCH(count_collocated),
    }, 10);
}