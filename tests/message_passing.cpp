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

#include <numeric>
#include <tests/test-utils.hh>
#include <core/thread.hh>
#include <ultramarine/actor.hpp>
#include <ultramarine/actor_ref.hpp>

struct no_copy_message {
    int count = 0;

    no_copy_message() = default;

    no_copy_message(no_copy_message const &) = delete;

    no_copy_message &operator=(no_copy_message const &other) = delete;

    no_copy_message(no_copy_message &&other) noexcept {
        count = other.count + 1;
    };

    no_copy_message &operator=(no_copy_message &&other) noexcept {
        count = other.count + 1;
        return *this;
    };
};

class counter_actor : public ultramarine::actor<counter_actor> {
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

    seastar::future<seastar::shard_id> get_execution_shard() const {
        return seastar::make_ready_future<seastar::shard_id>(seastar::engine().cpu_id());
    }

    int get_counter_int() const {
        return counter;
    }

    void move_arg_message(no_copy_message arg) const {
    }

    no_copy_message move_return_value_message() const {
        return no_copy_message();
    }

    seastar::future<no_copy_message> move_return_future_message() const {
        return seastar::make_ready_future<no_copy_message>(no_copy_message());
    }

    ULTRAMARINE_DEFINE_ACTOR(counter_actor,
                             (get_execution_shard)
                                     (increase_counter_future)(increase_counter_void)
                                     (get_counter_future)(get_counter_int)
                                     (move_arg_message)(move_return_value_message)(move_return_future_message));
};

using namespace seastar;

/*
 * Local
 */

SEASTAR_THREAD_TEST_CASE (ensure_same_core_location) {
    auto counterActor = ultramarine::get<counter_actor>(0);

    BOOST_CHECK(counterActor.tell(counter_actor::message::get_execution_shard()).get0() == seastar::engine().cpu_id());
}

SEASTAR_THREAD_TEST_CASE (same_core_mutating_future_message_passing) {
    auto counterActor = ultramarine::get<counter_actor>(0);

    auto ival = counterActor.tell(counter_actor::message::get_counter_int()).get0();
    counterActor.tell(counter_actor::message::increase_counter_future()).wait();
    auto nval = counterActor.tell(counter_actor::message::get_counter_int()).get0();

    BOOST_REQUIRE(nval == ival + 1);
}

SEASTAR_THREAD_TEST_CASE (same_core_mutating_void_message_passing) {
    auto counterActor = ultramarine::get<counter_actor>(0);

    auto ival = counterActor.tell(counter_actor::message::get_counter_int()).get0();
    counterActor.tell(counter_actor::message::increase_counter_void()).wait();
    auto nval = counterActor.tell(counter_actor::message::get_counter_int()).get0();

    BOOST_REQUIRE(nval == ival + 1);
}

SEASTAR_THREAD_TEST_CASE (same_core_nocopy_arg_message_passing) {
    ultramarine::get<counter_actor>(0).tell(counter_actor::message::move_arg_message(), no_copy_message()).wait();
}

SEASTAR_THREAD_TEST_CASE (same_core_nocopy_value_return_message_passing) {
    auto counterActor = ultramarine::get<counter_actor>(0);

    auto ret = counterActor.tell(counter_actor::message::move_return_value_message()).get0();
}

SEASTAR_THREAD_TEST_CASE (same_core_nocopy_future_return_message_passing) {
    auto counterActor = ultramarine::get<counter_actor>(0);

    auto ret = counterActor.tell(counter_actor::message::move_return_future_message()).get0();
}

/*
 * Collocated
 */

SEASTAR_THREAD_TEST_CASE (ensure_collocated_core_location) {
    auto counterActor = ultramarine::get<counter_actor>(1);

    BOOST_CHECK(counterActor.tell(counter_actor::message::get_execution_shard()).get0() != seastar::engine().cpu_id());
}

SEASTAR_THREAD_TEST_CASE (collocated_core_mutating_future_message_passing) {
    auto counterActor = ultramarine::get<counter_actor>(1);

    auto ival = counterActor.tell(counter_actor::message::get_counter_int()).get0();
    counterActor.tell(counter_actor::message::increase_counter_future()).wait();
    auto nval = counterActor.tell(counter_actor::message::get_counter_int()).get0();

    BOOST_REQUIRE(nval == ival + 1);
}

SEASTAR_THREAD_TEST_CASE (collocated_core_mutating_void_message_passing) {
    auto counterActor = ultramarine::get<counter_actor>(1);

    auto ival = counterActor.tell(counter_actor::message::get_counter_int()).get0();
    counterActor.tell(counter_actor::message::increase_counter_void()).wait();
    auto nval = counterActor.tell(counter_actor::message::get_counter_int()).get0();

    BOOST_REQUIRE(nval == ival + 1);
}

SEASTAR_THREAD_TEST_CASE (collocated_core_nocopy_arg_message_passing) {
    ultramarine::get<counter_actor>(1).tell(counter_actor::message::move_arg_message(), no_copy_message()).wait();
}

SEASTAR_THREAD_TEST_CASE (collocated_core_nocopy_value_return_message_passing) {
    auto counterActor = ultramarine::get<counter_actor>(1);

    auto ret = counterActor.tell(counter_actor::message::move_return_value_message()).get0();
}

SEASTAR_THREAD_TEST_CASE (collocated_core_nocopy_future_return_message_passing) {
    auto counterActor = ultramarine::get<counter_actor>(1);

    auto ret = counterActor.tell(counter_actor::message::move_return_future_message()).get0();
}