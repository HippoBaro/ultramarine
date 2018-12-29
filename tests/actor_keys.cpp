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
#include <ultramarine/actor_ref.hpp>
#include <ultramarine/actor.hpp>

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

class string_actor : public ultramarine::actor<string_actor> {

    std::string get_key() {
        return key;
    }

public:
    using KeyType = std::string;

    ULTRAMARINE_DEFINE_ACTOR(string_actor, (get_key))
};

using namespace seastar;

/*
 * Local
 */

SEASTAR_THREAD_TEST_CASE (key_value_rvref_litteral_preserved) {
    auto counterActor = ultramarine::get<string_actor>("test-actor-string-key");

    BOOST_CHECK(counterActor.tell(string_actor::message::get_key()).get0() == "test-actor-string-key");
}

SEASTAR_THREAD_TEST_CASE (key_value_rvref_string_preserved) {
    auto counterActor = ultramarine::get<string_actor>(std::string("test-actor-string-key"));

    BOOST_CHECK(counterActor.tell(string_actor::message::get_key()).get0() == "test-actor-string-key");
}

SEASTAR_THREAD_TEST_CASE (key_value_lvref_litteral_preserved) {
    auto key = "test-actor-string-key";
    auto counterActor = ultramarine::get<string_actor>(key);
    key = "test-actor-string-key2";

    BOOST_CHECK(counterActor.tell(string_actor::message::get_key()).get0() == "test-actor-string-key");
}

SEASTAR_THREAD_TEST_CASE (key_value_lvref_string_preserved) {
    auto key = std::string("test-actor-string-key");
    auto counterActor = ultramarine::get<string_actor>(key);
    key = std::string("test-actor-string-key2");

    BOOST_CHECK(counterActor.tell(string_actor::message::get_key()).get0() == "test-actor-string-key");
}