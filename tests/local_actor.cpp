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

class actor1 : public ultramarine::local_actor {
    volatile int i = 0;

    void stall() {
        while (i < std::numeric_limits<int>::max() / 10) i++;
        i = 0;
    }
public:
    static constexpr std::size_t max_activations = 1;

ULTRAMARINE_DEFINE_LOCAL_ACTOR(actor1, (stall))
};

ULTRAMARINE_IMPLEMENT_LOCAL_ACTOR(actor1);

class actor2 : public ultramarine::local_actor {
    volatile int i = 0;

    void stall() {
        while (i < std::numeric_limits<int>::max() / 10) i++;
        i = 0;
    }

ULTRAMARINE_DEFINE_LOCAL_ACTOR(actor2, (stall))
};

ULTRAMARINE_IMPLEMENT_LOCAL_ACTOR(actor2);

using namespace seastar;

/*
 * Local
 */

SEASTAR_THREAD_TEST_CASE (local_actor_scheduling) {
    BOOST_WARN(seastar::smp::count > 1);

    auto counterActor1 = ultramarine::get<actor1>(0);

    auto start1 = std::chrono::steady_clock::now();
    seastar::when_all(
            counterActor1.tell(actor1::message::stall()),
            counterActor1.tell(actor1::message::stall()),
            counterActor1.tell(actor1::message::stall()),
            counterActor1.tell(actor1::message::stall())
    ).wait();
    auto end1 = std::chrono::steady_clock::now();

    auto counterActor2 = ultramarine::get<actor2>(0);
    auto start2 = std::chrono::steady_clock::now();
    seastar::when_all(
            counterActor2.tell(actor2::message::stall()),
            counterActor2.tell(actor2::message::stall()),
            counterActor2.tell(actor2::message::stall()),
            counterActor2.tell(actor2::message::stall())
    ).wait();
    auto end2 = std::chrono::steady_clock::now();

    auto time1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    BOOST_CHECK(float(time1.count()) / time2.count() > 1.5f);
}