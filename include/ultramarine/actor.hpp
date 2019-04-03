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

#include <memory>
#include <unordered_map>
#include <boost/core/noncopyable.hpp>
#include <seastar/core/reactor.hh>
#include "impl/macro.hpp"

namespace ultramarine {

    using actor_id = std::size_t;
    using actor_activation_id = unsigned int;

    template<typename Actor>
    using directory = std::unordered_map<actor_id, Actor>;

    template<typename Actor>
    using ActorKey = typename Actor::KeyType;

    template<typename Actor>
    struct vtable {
        static constexpr auto table = Actor::message::make_vtable();
    };

    template<typename Derived>
    struct actor : private boost::noncopyable {
        using KeyType = actor_id;

        static thread_local std::unique_ptr<ultramarine::directory<Derived>> directory;

        static seastar::future<> clear_directory() {
            return seastar::smp::invoke_on_all([] {
                if (directory) {
                    directory->clear();
                }
                return seastar::make_ready_future();
            });
        }
    };

    template<typename Derived> thread_local std::unique_ptr<directory<Derived>> actor<Derived>::directory;
}

#include "actor_traits.hpp"
