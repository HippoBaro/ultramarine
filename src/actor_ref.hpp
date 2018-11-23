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
    template<typename Actor>
    struct vtable {
        static constexpr auto table = Actor::message::make_vtable();
    };

    template<typename Actor>
    class collocated_actor_ref {
        seastar::shard_id loc;

    public:
        explicit collocated_actor_ref(seastar::shard_id loc) : loc(loc) {}

        template<typename Handler>
        auto schedule(actor_id id, Handler message) const {
            return seastar::smp::submit_to(loc, [id, message] {
                return (hold_activation<Actor>(id)->*vtable<Actor>::table[message])();
            });
        }

        template<typename Handler, typename ...Args>
        auto schedule(actor_id id, Handler message, Args &&... args) const {
            return seastar::smp::submit_to(loc, [=] {
                return (hold_activation<Actor>(id)->*vtable<Actor>::table[message])(args...);
            });
        }
    };

    template<typename Actor>
    class local_actor_ref {
        Actor *inst = nullptr;

    public:
        explicit local_actor_ref(actor_id id) : inst(hold_activation<Actor>(id)) {};

        template<typename Handler>
        auto schedule(actor_id id, Handler message) const {
            return (inst->*vtable<Actor>::table[message])();
        }

        template<typename Handler, typename ...Args>
        auto schedule(actor_id id, Handler message, Args &&... args) const {
            return (inst->*vtable<Actor>::table[message])(args...);
        }
    };

    template<typename Actor>
    using actor_ref_impl = std::variant<local_actor_ref<Actor>, collocated_actor_ref<Actor>>;

    class actor_directory {
    public:
        template<typename Actor>
        static actor_ref_impl<Actor> locate(actor_id id) {
            auto shard = id % seastar::smp::count;
            if (shard == seastar::engine().cpu_id()) {
                return local_actor_ref<Actor>(id);
            } else {
                return collocated_actor_ref<Actor>{shard};
            }
        }
    };

    template<typename Actor>
    struct actor_ref {
        actor_id ref;
        actor_ref_impl<Actor> impl;

        explicit actor_ref(actor_id a_id) : ref(a_id), impl(actor_directory::locate<Actor>(ref)) {}

        actor_ref(actor_ref const &) = default;

        actor_ref(actor_ref &&) noexcept = default;

        template<typename Handler, typename ...Args>
        auto inline tell(Handler message, Args &&... args) const {
            return [this, message, args = std::make_tuple(std::forward<Args>(args) ...)]() mutable {
                return std::visit([this, message, args = std::move(args)](auto &&impl) {
                    return std::apply([this, message, &impl](auto &&... args) {
                        using ret_type = std::result_of_t<decltype(vtable<Actor>::table[message])(Actor *)>;
                        if constexpr (!seastar::is_future<ret_type>::value) {
                            return seastar::futurize<ret_type>::apply([&impl, message, this, &args...] {
                                impl.schedule(ref, message, args...);
                            });
                        }
                        else {
                            return impl.schedule(ref, message, args...);
                        }
                    }, std::move(args));
                }, impl);

            }();
        }

        template<typename Handler>
        auto inline tell(Handler message) const ->
        seastar::futurize_t<std::result_of_t<decltype(vtable<Actor>::table[message])(Actor *)>> {
            return std::visit([this, message](auto &&impl) {
                using ret_type = std::result_of_t<decltype(vtable<Actor>::table[message])(Actor *)>;
                if constexpr (!seastar::is_future<ret_type>::value) {
                    if constexpr (std::is_void<ret_type>::value) {
                        return seastar::futurize<ret_type>::apply([&impl, message, this] { impl.schedule(ref, message);});
                    }
                    else {
                        return seastar::futurize<ret_type>::apply([&impl, message, this] { return impl.schedule(ref, message);});
                    }
                }
                else {
                    return impl.schedule(ref, message);
                }
            }, impl);
        };
    };

    template<typename Actor>
    actor_ref<Actor> get(actor_id id) {
        return actor_ref<Actor>(id);
    }
}
