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

#include <tuple>
#include <memory>
#include <unordered_map>
#include <boost/core/noncopyable.hpp>
#include <boost/hana.hpp>
#include <boost/hana/string.hpp>

#include <core/future.hh>

namespace ultramarine {

    using actor_id = unsigned int;
    using actor_activation_id = unsigned int;

    class actor : private boost::noncopyable {
    public:

        const actor_id id;

        explicit actor(actor_id id) : id(id) { }
    };

    template <typename Actor>
    struct actor_activation : private boost::noncopyable {
        explicit actor_activation(actor_id id) : id(id), instance(id) {}

        actor_activation_id id;
        Actor instance;
    };

    template <typename ActorKind>
    using directory = std::unordered_map<actor_id, std::optional<actor_activation<ActorKind>>>;

    template <typename Actor>
    class actor_ref {
        actor_id ref;
        actor_activation<Actor> * inst = nullptr;

    public:
        explicit actor_ref(actor_id a_id) : ref(a_id) {}
        actor_ref(actor_ref const&) = default;
        actor_ref(actor_ref &&) noexcept = default;

        actor_activation<Actor> *hold() {
            if (inst) [[likely]] {
                return inst;
            }
            else if (!Actor::directory) {
                Actor::directory = std::make_unique<ultramarine::directory<Actor>>();
            }

            auto &r = (*Actor::directory)[ref];

            if (!r) {
                seastar::print("Creating actor with id %u on core %u\n", ref, seastar::engine().cpu_id());
                r.emplace(ref);
            }
            return inst = &(*r);
        }

        seastar::future<seastar::shard_id> lives_in() {
            return seastar::make_ready_future<seastar::shard_id>(seastar::engine().cpu_id());
        }

        seastar::future<> say_hello() {
            return hold()->instance.say_hello();
        }
    };
}