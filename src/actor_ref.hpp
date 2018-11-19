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

#include <variant>
#include <core/print.hh>
#include <core/reactor.hh>
#include "actor.hpp"

namespace ultramarine {
    template <typename Actor>
    actor_activation<Actor> *hold_activation(actor_id id) {
        if (!Actor::directory) {
            Actor::directory = std::make_unique<ultramarine::directory<Actor>>();
        }

        auto &r = (*Actor::directory)[id];
        if (!r) {
            seastar::print("Creating actor with id %u on core %u\n", id, seastar::engine().cpu_id());
            r.emplace(id);
        }
        return &(*r);
    }

    template <typename Actor>
    class collocated_actor_ref {
        seastar::shard_id loc;

    public:
        explicit collocated_actor_ref(seastar::shard_id loc) : loc(loc) {}

        template <typename Callable>
        auto schedule(actor_id id, Callable &&call) {
            seastar::print("Calling on collocated shard\n");
            return seastar::smp::submit_to(loc, [call = std::move(call), id] {
                return call(hold_activation<Actor>(id)->instance);
            });
        }
    };

    template <typename Actor>
    class local_actor_ref {
        actor_activation<Actor> * inst = nullptr;

        actor_activation<Actor> *hold(actor_id id) {
            if (inst) [[likely]] {
                return inst;
            }

            return inst = hold_activation<Actor>(id);
        }

    public:

        template <typename Callable>
        auto schedule(actor_id id, Callable &&call) {
            seastar::print("Calling on local shard\n");
            return call(hold(id)->instance);
        }
    };

    template <typename Actor>
    using actor_ref_impl = std::variant<local_actor_ref<Actor>, collocated_actor_ref<Actor>>;

    class actor_directory {
    public:
        template <typename Actor>
        static actor_ref_impl<Actor> locate(actor_id id) {
            auto shard = id % seastar::smp::count;
            if (shard == seastar::engine().cpu_id()) {
                return local_actor_ref<Actor>{};
            }
            else {
                return collocated_actor_ref<Actor>{shard};
            }
        }
    };

    template<typename Actor>
    struct vtable {
        static constexpr auto table = Actor::ultramarine_handlers();
    };

    template <typename Actor>
    struct actor_ref {
        actor_id ref;
        actor_ref_impl<Actor> impl;

        explicit actor_ref(actor_id a_id) : ref(a_id), impl(actor_directory::locate<Actor>(ref)) {}
        actor_ref(actor_ref const&) = default;
        actor_ref(actor_ref &&) noexcept = default;

        template <typename Handler>
        auto tell(Handler message) {
            return std::visit([this, message](auto&& arg) {
                return arg.schedule(ref, [message] (auto &actor) {
                    return vtable<Actor>::table[message](actor);
                });
            }, impl);
        }
    };
}
