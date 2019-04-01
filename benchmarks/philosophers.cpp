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
#include "benchmark_utility.hpp"

static constexpr std::size_t PhilosopherLen = 20;
static constexpr std::size_t RoundLen = 16000;

class arbitrator_actor : public ultramarine::actor<arbitrator_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(arbitrator_actor, (hungry)(done));
    std::array<bool, PhilosopherLen> forks{};

    seastar::future<bool> hungry(int philosopher_index);

    void done(int philosopher_index);
};

class philosopher_actor : public ultramarine::actor<philosopher_actor> {
public:
ULTRAMARINE_DEFINE_ACTOR(philosopher_actor, (start));
    int round = 0;
    ultramarine::actor_ref<arbitrator_actor> arbitrator = ultramarine::get<arbitrator_actor>(0);

    seastar::future<> start();
};

seastar::future<bool> arbitrator_actor::hungry(int philosopher_index) {
    auto &leftFork = forks[philosopher_index];
    auto &rightFork = forks[(philosopher_index + 1) % PhilosopherLen];

    if (leftFork || rightFork) {
        return seastar::make_ready_future<bool>(false);
    } else {
        leftFork = true;
        rightFork = true;
        return seastar::make_ready_future<bool>(true);
    }
}

void arbitrator_actor::done(int philosopher_index) {
    forks[philosopher_index] = false;
    forks[(philosopher_index + 1) % PhilosopherLen] = false;
}

seastar::future<> philosopher_actor::start() {
    return seastar::do_until([this] { return round >= RoundLen; }, [this] {
        return arbitrator.tell(arbitrator_actor::message::hungry(), this->key).then([this](bool allowed) {
            if (allowed) {
                ++round;
                return arbitrator.tell(arbitrator_actor::message::done(), this->key);
            }
            return seastar::make_ready_future();
        });
    });
}

seastar::future<> dinning_philosophers() {
    std::vector<seastar::future<>> futs;
    for (int j = 0; j < PhilosopherLen; ++j) {
        futs.emplace_back(ultramarine::get<philosopher_actor>(j).tell(philosopher_actor::message::start()));
    }
    return seastar::when_all(std::begin(futs), std::end(futs)).discard_result();
}

int main(int ac, char **av) {
    return ultramarine::benchmark::run(ac, av, {
            ULTRAMARINE_BENCH(dinning_philosophers),
    }, 1);
}