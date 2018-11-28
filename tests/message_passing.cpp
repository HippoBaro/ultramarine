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

#include "unit-testing.hpp"
#include "actor.hpp"
#include "actor_ref.hpp"
#include "macro.hpp"

class counter_actor : ultramarine::actor {
public:
    int counter = 0;

    seastar::future<> increase_counter_future() {
        counter++;
        return seastar::make_ready_future();
    }

    void increase_counter_void() {
        counter++;
    }

    seastar::future<int> get_counter_future() const {
        return seastar::make_ready_future<int>(counter);
    }

    int get_counter_int() const {
        return counter;
    }

    seastar::future<int> accumulate_future(std::vector<int> const& pack) const {
        return seastar::make_ready_future<int>(std::accumulate(std::begin(pack), std::end(pack), 0));
    }

    int accumulate_value(std::vector<int> const& pack) const {
        return std::accumulate(std::begin(pack), std::end(pack), 0);
    }

    ULTRAMARINE_DEFINE_ACTOR(counter_actor,
                             (increase_counter_future)(increase_counter_void)
                             (get_counter_future)(get_counter_int)
                             (accumulate_future)(accumulate_value));
};
ULTRAMARINE_IMPLEMENT_ACTOR(counter_actor);

TEST_CASE ("Same-core mutating future message passing") {
    auto counterActor = ultramarine::get<counter_actor>(0);
    counterActor.tell(counter_actor::message::increase_counter_future()).then([counterActor] {
        return counterActor.tell(counter_actor::message::get_counter_int()).then([] (int value) {
            REQUIRE(value == 1);
        });
    }).wait();
}

TEST_CASE ("Same-core mutating void message passing") {
    auto counterActor = ultramarine::get<counter_actor>(0);
    counterActor.tell(counter_actor::message::increase_counter_void()).then([counterActor] {
        return counterActor.tell(counter_actor::message::get_counter_int()).then([] (int value) {
            REQUIRE(value == 1);
        });
    }).wait();
}