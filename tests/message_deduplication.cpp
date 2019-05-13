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
#include <seastar/testing/thread_test_case.hh>
#include <seastar/core/thread.hh>
#include <ultramarine/actor.hpp>
#include <ultramarine/actor_ref.hpp>
#include <ultramarine/impl/message_deduplicate.hpp>

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
    ULTRAMARINE_DEFINE_ACTOR(counter_actor,(noop_nocopy)(exception_nocopy));

public:

    seastar::future<no_copy_message> noop_nocopy(no_copy_message const&) const {
        return seastar::make_ready_future<no_copy_message>();
    }

    seastar::future<no_copy_message> exception_nocopy(no_copy_message const&) const {
        throw std::runtime_error("error");
    }

};

using namespace seastar;

/*
 * Local
 */

SEASTAR_THREAD_TEST_CASE (ensure_local_nocopy_message_packing) {
    auto counterActor = ultramarine::get<counter_actor>(0);
    ultramarine::impl::deduplicate(counterActor, counter_actor::message::noop_nocopy(), [](auto &d) {
        d(no_copy_message());
        return seastar::make_ready_future();
    }).then([](auto &&vec) {
        static_assert(std::is_same_v<typename std::decay_t<decltype(vec)>::value_type,no_copy_message>);
        BOOST_CHECK(std::size(vec) == 1);
        return seastar::make_ready_future();
    }).wait();
}

SEASTAR_THREAD_TEST_CASE (ensure_local_exception_message_packing) {
    auto counterActor = ultramarine::get<counter_actor>(0);
    ultramarine::impl::deduplicate(counterActor, counter_actor::message::exception_nocopy(), [](auto &d) {
        d(no_copy_message());
        return seastar::make_ready_future();
    }).then([](auto &&vec) {
        static_assert(std::is_same_v<typename std::decay_t<decltype(vec)>::value_type,no_copy_message>);
        BOOST_FAIL("received response, expected exception");
        return seastar::make_ready_future();
    }).handle_exception_type([] (std::runtime_error const&ex) {
        BOOST_CHECK(std::strcmp(ex.what(), "error") == 0);
    }).wait();
}

/*
 * Collocated
 */

SEASTAR_THREAD_TEST_CASE (ensure_collocated_nocopy_message_packing) {
    auto counterActor = ultramarine::get<counter_actor>(1);
    ultramarine::impl::deduplicate(counterActor, counter_actor::message::noop_nocopy(), [](auto &d) {
        d(no_copy_message());
        return seastar::make_ready_future();
    }).then([](auto &&vec) {
        static_assert(std::is_same_v<typename std::decay_t<decltype(vec)>::value_type,no_copy_message>);
        BOOST_CHECK(std::size(vec) == 1);
        return seastar::make_ready_future();
    }).wait();
}

SEASTAR_THREAD_TEST_CASE (ensure_collocated_exception_message_packing) {
    auto counterActor = ultramarine::get<counter_actor>(1);
    ultramarine::impl::deduplicate(counterActor, counter_actor::message::exception_nocopy(), [](auto &d) {
        d(no_copy_message());
        return seastar::make_ready_future();
    }).then([](auto &&vec) {
        static_assert(std::is_same_v<typename std::decay_t<decltype(vec)>::value_type,no_copy_message>);
        BOOST_FAIL("received response, expected exception");
        return seastar::make_ready_future();
    }).handle_exception_type([] (std::runtime_error const&ex) {
        BOOST_CHECK(std::strcmp(ex.what(), "error") == 0);
    }).wait();
}