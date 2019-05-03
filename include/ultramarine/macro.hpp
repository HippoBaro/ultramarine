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
#include <boost/hana.hpp>
#include <seastar/core/future.hh>

/// \exclude
#define ULTRAMARINE_LITERAL(lit) #lit

/// \exclude
#define ULTRAMARINE_MAKE_TAG(a, b, i, tag)                                                                  \
static constexpr auto tag() { return BOOST_HANA_STRING(ULTRAMARINE_LITERAL(tag)); }                         \

/// \exclude
#define ULTRAMARINE_MAKE_TAG_ALT(a, b, i, tag)                                                              \
template<typename ...Args, class T = b>                                                                     \
inline constexpr auto tag(Args &&... args) const                                                            \
  -> seastar::futurize_t<std::result_of_t <decltype(&T::tag)(T, Args...)>> {                                \
    return ref.tell(BOOST_HANA_STRING(ULTRAMARINE_LITERAL(tag)), std::forward<Args>(args) ...);             \
}                                                                                                           \
template<class T = b>                                                                                       \
inline constexpr auto tag() const -> seastar::futurize_t<std::result_of_t <decltype(&T::tag)(T)>> {         \
    return ref.tell(BOOST_HANA_STRING(ULTRAMARINE_LITERAL(tag)));                                           \
}                                                                                                           \


/// \exclude
#define ULTRAMARINE_MAKE_TUPLE(a, data, i, name)                                                            \
    boost::hana::make_pair(internal::message::name(), &data::name),                                                   \


/// \brief Expands with enclosing actor internal definitions
///
/// Example:
/// ```cpp
/// class simple_actor : public ultramarine::actor<simple_actor> {
/// public:
///     seastar::future<> my_message() const;
///     seastar::future<> another_message() const;
///
///     ULTRAMARINE_DEFINE_ACTOR(simple_actor, (my_message)(another_message));
/// };
/// ```
/// \unique_name ULTRAMARINE_DEFINE_ACTOR
/// \requires `name` shall be a [ultramarine::actor]() derived type
/// \requires `seq` shall be a sequence of zero or more message handler (Example: `(handler1)(handler2)`)
#define ULTRAMARINE_DEFINE_ACTOR(name, seq)                                                                 \
private:                                                                                                    \
      const KeyType key;                                                                                    \
public:                                                                                                     \
      explicit name(KeyType key) : key(std::move(key)) { }                                                  \
      struct internal {                                                                                     \
          template<typename Ref>                                                                            \
          struct interface {                                                                                \
              static_assert(std::is_same_v<typename Ref::ActorType, name>,                                  \
                    "actor_ref used with invalid actor type");                                              \
              Ref const& ref;                                                                               \
              explicit interface(Ref const& ref) : ref(ref) {}                                              \
              explicit interface(interface const&) = delete;                                                \
              explicit interface(interface &&) = delete;                                                    \
              BOOST_PP_SEQ_FOR_EACH_I(ULTRAMARINE_MAKE_TAG_ALT, name, seq)                                  \
              constexpr auto operator->(){ return this; }                                                   \
          };                                                                                                \
          struct message {                                                                                  \
              BOOST_PP_SEQ_FOR_EACH_I(ULTRAMARINE_MAKE_TAG, name, seq)                                      \
          private:                                                                                          \
              friend class ultramarine::impl::vtable<name>;                                                 \
              static constexpr auto make_vtable() {                                                         \
                  return boost::hana::make_map(                                                             \
                      BOOST_PP_SEQ_FOR_EACH_I(ULTRAMARINE_MAKE_TUPLE, name, seq)                            \
                      boost::hana::make_pair(BOOST_HANA_STRING("ultramarine_dummy"), nullptr)               \
                  );                                                                                        \
              }                                                                                             \
          };                                                                                                \
      };                                                                                                    \
      using message = internal::message;  /* FIXME: workaround */                                           \

