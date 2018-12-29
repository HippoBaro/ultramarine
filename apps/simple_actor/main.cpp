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

#include <seastar/core/app-template.hh>
#include <ultramarine/silo.hpp>
#include <ultramarine/actor.hpp>
#include <ultramarine/actor_ref.hpp>

class simple_actor : public ultramarine::actor<simple_actor> {
public:
    using KeyType = std::string;

    seastar::future<> say_hello() const {
        seastar::print("Hello, World; from simple_actor %s (%zu bytes) located on core %u.\n", key, sizeof(simple_actor),
                       seastar::engine().cpu_id());
        return seastar::make_ready_future();
    }

    ULTRAMARINE_DEFINE_ACTOR(simple_actor, (say_hello));
};

int main(int ac, char **av) {
    seastar::app_template app;

    return app.run(ac, av, [] {
        auto silo = new ultramarine::silo_server();
        return silo->start().then([silo] {
            seastar::engine().at_exit([silo] {
                return silo->stop().then([silo] {
                    delete silo;
                    return seastar::make_ready_future();
                });
            });

            return ultramarine::get<simple_actor>("Ultra").tell(simple_actor::message::say_hello());
        });
    });
}