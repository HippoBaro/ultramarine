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

#pragma once

#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/hana.hpp>

#define ULTRAMARINE_MAKE_TAG(a, b, i, tag)                                                                  \
    static constexpr auto tag() { return BOOST_HANA_STRING(#tag); }                                         \

#define ULTRAMARINE_MAKE_TUPLE(a, data, i, name)                                                            \
    boost::hana::make_pair(name(), &data::name),                                                            \

/*
    Example:
        ULTRAMARINE_DEFINE_ACTOR(simple_actor, (method1)(method2))

    expands to:

    using ultramarine::actor::actor;
    friend class ultramarine::actor_ref<simple_actor>;
    friend class ultramarine::actor_activation<simple_actor>;

public:
    struct message {
        static constexpr auto method1() { return BOOST_HANA_STRING("method1"); }
        static constexpr auto method2() { return BOOST_HANA_STRING("method2"); }

private:
        friend class ultramarine::vtable<simple_actor>;
        static constexpr auto make_vtable() {
            using namespace ultramarine::literals;
            return boost::hana::make_map(
                    boost::hana::make_pair(method1(), &simple_actor::method1),
                    boost::hana::make_pair(method2(), &simple_actor::method2),
                    boost::hana::make_pair(BOOST_HANA_STRING("dummy"), nullptr)
            );
        }
    };
    static thread_local std::unique_ptr<ultramarine::directory<simple_actor>> directory;
 */
#define ULTRAMARINE_DEFINE_ACTOR(name, seq)                                                                 \
      using ultramarine::actor::actor;                                                                      \
      friend class ultramarine::actor_ref<name>;                                                            \
      friend class ultramarine::actor_activation<name>;                                                     \
public:                                                                                                     \
      struct message {                                                                                      \
          BOOST_PP_SEQ_FOR_EACH_I(ULTRAMARINE_MAKE_TAG, name, seq)                                          \
private:                                                                                                    \
    friend class ultramarine::vtable<name>;                                                                 \
    static constexpr auto make_vtable() {                                                                   \
      return boost::hana::make_map(                                                                         \
        BOOST_PP_SEQ_FOR_EACH_I(ULTRAMARINE_MAKE_TUPLE, name, seq)                                          \
        boost::hana::make_pair(BOOST_HANA_STRING("dummy"), nullptr)                                         \
      );                                                                                                    \
    }                                                                                                       \
};                                                                                                          \
static thread_local std::unique_ptr<ultramarine::directory<name>> directory;

#define ULTRAMARINE_IMPLEMENT_ACTOR(name)                                                                   \
    thread_local std::unique_ptr<ultramarine::directory<name>> name::directory;                             \

