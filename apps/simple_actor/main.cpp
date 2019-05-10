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
#include <seastar/core/print.hh>
#include <ultramarine/actor_ref.hpp>
#include "simple_actor.hpp"

int main(int ac, char **av) {
    fmt::print("actor_ref size: {}\n", sizeof(ultramarine::actor_ref<simple_actor>));
    fmt::print(" -- local_actor_ref size: {}\n", sizeof(ultramarine::impl::collocated_actor_ref<simple_actor>));

    fmt::print("actor size: {}\n", sizeof(simple_actor));
    fmt::print(" -- key size: {}\n", sizeof(simple_actor::KeyType));

    seastar::app_template app;
    return app.run(ac, av, [] {
        return ultramarine::get<simple_actor>("Ultra")->say_hello();
    });
}