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

class simple_actor : ultramarine::actor {
    seastar::future<> method1() {
        seastar::print("Hello, method 1 in core %u\n", seastar::engine().cpu_id());
        return seastar::make_ready_future();
    }

    seastar::future<> method2(std::string_view arg1, int arg2) {
        seastar::print("Hello, method 2 in core %u; Arg1 is %s, arg2 is %d\n", seastar::engine().cpu_id(), arg1, arg2);
        return seastar::make_ready_future();
    }

    ULTRAMARINE_DEFINE_ACTOR(simple_actor, (method1)(method2));
};
ULTRAMARINE_IMPLEMENT_ACTOR(simple_actor);

int main(int ac, char **av) {
    seastar::app_template app;

    printf("simple_actor size: %zu\n", sizeof(simple_actor));

    return app.run(ac, av, [] {
        auto silo = new ultramarine::silo_server();
        return silo->start().then([silo] {
            seastar::engine().at_exit([silo] {
                seastar::print("at exit\n");
                return silo->stop();
            });

            auto ref = ultramarine::get_actor<simple_actor>(0);
            return seastar::do_with(std::move(ref), [](auto &ref) {
                using namespace ultramarine::literals;
                return ref.tell(simple_actor::message::method1()).then([&ref] {
                    return ref.tell(simple_actor::message::method2(), "toto", 1);
                });
            });
        });
    });
}