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

#include <chrono>

#include "core/app-template.hh"
#include "silo.hpp"
#include "actor.hpp"

class simple_actor : public ultramarine::actor {
    using ultramarine::actor::actor;

    int counter = 0;

    auto register_handlers() const {
        boost::hana::make_map(
                boost::hana::make_pair(BOOST_HANA_STRING("method1"), []() {
                    seastar::print("Hello, method 1\n");
                }),
                boost::hana::make_pair(BOOST_HANA_STRING("method2"), []() {
                    seastar::print("Hello, method 2\n");
                })
        );
    }

public:
    seastar::future<> say_hello() {
        //++counter;
        return seastar::make_ready_future();
    }

    int get() const {
        return counter;
    }

public:
    static thread_local std::unique_ptr<ultramarine::directory<simple_actor>> directory;
};

thread_local std::unique_ptr<ultramarine::directory<simple_actor>> simple_actor::directory;

int main(int ac, char **av) {
    seastar::app_template app;

    return app.run(ac, av, [] {
        auto silo = new ultramarine::silo_server();
        return silo->start().then([silo] {
            seastar::engine().at_exit([silo] {
                seastar::print("at exit\n");
                return silo->stop();
            });

            auto ref = ultramarine::get_actor<simple_actor>(42);
            return seastar::do_with(std::move(ref), [](auto &ref) {
                return ref.say_hello().then([&] {
                    return ref.lives_in().then([](auto cpu_id) {
                        seastar::print("actor lives in shard %u\n", cpu_id);
                    });
                });
            }).then([] {
                auto ref = ultramarine::get_actor<simple_actor>(905);
                return ref.say_hello().then([&] {
                    return ref.lives_in().then([](auto cpu_id) {
                        seastar::print("actor lives in shard %u\n", cpu_id);
                    });
                });
            });
        });
    });
}

//ref instance took 156436 us
//ref instance took 1582817 us

//fetch instance took 831766 us
//fetch instance took 8356604 us