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

#include <ultramarine/actor_ref.hpp>
#include <ultramarine/actor.hpp>
#include <seastar/testing/thread_test_case.hh>

class error_actor : public ultramarine::actor<error_actor> {
public:
    void void_throws() {
        throw std::runtime_error("error");
    }

    seastar::future<> future_throws() {
        throw std::runtime_error("error");
    }

    seastar::future<> ex_future() {
        return seastar::make_exception_future(std::make_exception_ptr(std::runtime_error("error")));
    }

    ULTRAMARINE_DEFINE_ACTOR(error_actor, (void_throws)(future_throws)(ex_future))
};

using namespace seastar;

/*
 * Local
 */

SEASTAR_THREAD_TEST_CASE (local_core_actor_throws_void) {
        auto ref = ultramarine::get<error_actor>(0);

        ref.tell(error_actor::message::void_throws()).then([] {
            BOOST_FAIL("Exception escaped");
        }).handle_exception([] (std::exception_ptr eptr) {
            BOOST_CHECK(eptr);
            try {
                std::rethrow_exception(eptr);
            }
            catch (std::runtime_error const& ex) {
                BOOST_REQUIRE_EQUAL(ex.what(), "error");
            }
            catch (...) {
                BOOST_FAIL("Exception type was unexpected");
            }

        }).wait();
}

SEASTAR_THREAD_TEST_CASE (local_core_actor_throws_future) {
    auto ref = ultramarine::get<error_actor>(0);

    ref.tell(error_actor::message::future_throws()).then([] {
        BOOST_FAIL("Exception escaped");
    }).handle_exception([] (std::exception_ptr eptr) {
        BOOST_CHECK(eptr);
        try {
            std::rethrow_exception(eptr);
        }
        catch (std::runtime_error const& ex) {
            BOOST_REQUIRE_EQUAL(ex.what(), "error");
        }
        catch (...) {
            BOOST_FAIL("Exception type was unexpected");
        }
    }).wait();
}

SEASTAR_THREAD_TEST_CASE (local_core_actor_exceptionnal_future) {
    auto ref = ultramarine::get<error_actor>(0);

    ref.tell(error_actor::message::ex_future()).then([] {
        BOOST_FAIL("Exception escaped");
    }).handle_exception([] (std::exception_ptr eptr) {
        BOOST_CHECK(eptr);
        try {
            std::rethrow_exception(eptr);
        }
        catch (std::runtime_error const& ex) {
            BOOST_REQUIRE_EQUAL(ex.what(), "error");
        }
        catch (...) {
            BOOST_FAIL("Exception type was unexpected");
        }
    }).wait();
}

SEASTAR_THREAD_TEST_CASE (collocated_core_actor_throws_void) {
    auto ref = ultramarine::get<error_actor>(1);

    ref.tell(error_actor::message::void_throws()).then([] {
        BOOST_FAIL("Exception escaped");
    }).handle_exception([] (std::exception_ptr eptr) {
        BOOST_CHECK(eptr);
        try {
            std::rethrow_exception(eptr);
        }
        catch (std::runtime_error const& ex) {
            BOOST_REQUIRE_EQUAL(ex.what(), "error");
        }
        catch (...) {
            BOOST_FAIL("Exception type was unexpected");
        }

    }).wait();
}

SEASTAR_THREAD_TEST_CASE (collocated_core_actor_throws_future) {
    auto ref = ultramarine::get<error_actor>(1);

    ref.tell(error_actor::message::future_throws()).then([] {
        BOOST_FAIL("Exception escaped");
    }).handle_exception([] (std::exception_ptr eptr) {
        BOOST_CHECK(eptr);
        try {
            std::rethrow_exception(eptr);
        }
        catch (std::runtime_error const& ex) {
            BOOST_REQUIRE_EQUAL(ex.what(), "error");
        }
        catch (...) {
            BOOST_FAIL("Exception type was unexpected");
        }
    }).wait();
}

SEASTAR_THREAD_TEST_CASE (collocated_core_actor_exceptionnal_future) {
    auto ref = ultramarine::get<error_actor>(1);

    ref.tell(error_actor::message::ex_future()).then([] {
        BOOST_FAIL("Exception escaped");
    }).handle_exception([] (std::exception_ptr eptr) {
        BOOST_CHECK(eptr);
        try {
            std::rethrow_exception(eptr);
        }
        catch (std::runtime_error const& ex) {
            BOOST_REQUIRE_EQUAL(ex.what(), "error");
        }
        catch (...) {
            BOOST_FAIL("Exception type was unexpected");
        }
    }).wait();
}