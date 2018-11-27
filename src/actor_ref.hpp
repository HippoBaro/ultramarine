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

    template<typename Actor, typename Impl>
    struct actor_ref_impl {
        actor_id ref;

        template<typename Handler, typename ...Args>
        auto inline tell(Handler message, Args &&... args) const {

            using ret_type = std::result_of_t<decltype(vtable<Actor>::table[message])(Actor, Args ...)>;

            return [this, message, args = std::make_tuple(std::forward<Args>(args) ...)]() mutable {
                if constexpr (std::is_void<ret_type>::value) {
                    return std::apply([this, message](auto &&... args) {
                        static_cast<Impl const*>(this)->schedule(ref, message, args...);
                    }, std::move(args));
                } else if constexpr (!seastar::is_future<ret_type>::value) {
                    return seastar::futurize<ret_type>::apply([this, message, &args] {
                        return std::apply([this, message](auto &&... args) {
                            return static_cast<Impl const*>(this)->schedule(ref, message, args...);
                        }, std::move(args));
                    });
                } else {
                    return std::apply([this, message](auto &&... args) {
                        return static_cast<Impl const*>(this)->schedule(ref, message, args...);
                    }, std::move(args));
                }
            }();
        }

        template<typename Handler>
        auto inline tell(Handler message) const {
            using ret_type = std::result_of_t<decltype(vtable<Actor>::table[message])(Actor)>;

            if constexpr (std::is_void<ret_type>::value) {
                return seastar::futurize<ret_type>::apply([this, message] {
                    static_cast<Impl const*>(this)->schedule(ref, message);
                });
            } else if constexpr (!seastar::is_future<ret_type>::value) {
                return seastar::futurize<ret_type>::apply([this, message] {
                    return static_cast<Impl const*>(this)->schedule(ref, message);
                });
            } else {
                return static_cast<Impl const*>(this)->schedule(ref, message);
            }
        };
    };

    template<typename Actor>
    class collocated_actor_ref : public actor_ref_impl<Actor, collocated_actor_ref<Actor>> {
        seastar::shard_id loc;

    public:
        explicit collocated_actor_ref(actor_id ref, seastar::shard_id loc)
                : actor_ref_impl<Actor, collocated_actor_ref<Actor>>{ref}, loc(loc) {}

        using actor_ref_impl<Actor, collocated_actor_ref>::tell;

        template<typename Handler>
        auto schedule(actor_id id, Handler message) const {
            return seastar::smp::submit_to(loc, [id, message] {
                return (hold_activation<Actor>(id)->*vtable<Actor>::table[message])();
            });
        }

        template<typename Handler, typename ...Args>
        auto schedule(actor_id id, Handler message, Args &&... args) const {
            return [this, id, message, args = std::make_tuple(std::forward<Args>(args) ...)]() mutable {
                return seastar::smp::submit_to(loc, [id, message, args = std::move(args)] {
                    return std::apply([id, message](auto &&... args) {
                        return (hold_activation<Actor>(id)->*vtable<Actor>::table[message])(std::move(args)...);
                    }, std::move(args));
                });
            }();
        }
    };

//    AFTER AFTER
//    plain_object_future_args: 173 us (min: 171 us -- 99p: 181 us)
//    local_actor_future_args: 167 us (min: 166 us -- 99p: 170 us)
//    collocated_actor_future_args: 7011 us (min: 6846 us -- 99p: 7503 us)
//    plain_object_int_args: 69 us (min: 68 us -- 99p: 72 us)
//    local_actor_int_args: 171 us (min: 166 us -- 99p: 207 us)
//    collocated_actor_int_args: 7001 us (min: 6850 us -- 99p: 7304 us)

//    AFTER
//    plain_object_future_args: 176 us (min: 175 us -- 99p: 182 us)
//    local_actor_future_args: 254 us (min: 242 us -- 99p: 374 us)
//    collocated_actor_future_args: 7046 us (min: 6942 us -- 99p: 7165 us)
//    plain_object_int_args: 70 us (min: 68 us -- 99p: 74 us)
//    local_actor_int_args: 244 us (min: 239 us -- 99p: 250 us)
//    collocated_actor_int_args: 7059 us (min: 6931 us -- 99p: 7176 us)

//    BEFORE
//    plain_object_future_args: 177 us (min: 175 us -- 99p: 181 us)
//    local_actor_future_args: 247 us (min: 242 us -- 99p: 263 us)
//    collocated_actor_future_args: 8024 us (min: 7216 us -- 99p: 18987 us)
//    plain_object_int_args: 73 us (min: 67 us -- 99p: 108 us)
//    local_actor_int_args: 296 us (min: 276 us -- 99p: 442 us)
//    collocated_actor_int_args: 8956 us (min: 7299 us -- 99p: 24382 us)

    template<typename Actor>
    class local_actor_ref : public actor_ref_impl<Actor, local_actor_ref<Actor>> {
        Actor *inst = nullptr;

    public:
        explicit local_actor_ref(actor_id id) : actor_ref_impl<Actor, local_actor_ref<Actor>>{id},
                                                inst(hold_activation<Actor>(id)) {};
        using actor_ref_impl<Actor, local_actor_ref<Actor>>::tell;

        template<typename Handler>
        auto schedule(actor_id, Handler message) const {
            return (inst->*vtable<Actor>::table[message])();
        }

        template<typename Handler, typename ...Args>
        auto schedule(actor_id, Handler message, Args &&... args) const {
            return (inst->*vtable<Actor>::table[message])(std::move(args)...);
        }
    };

    template<typename Actor>
    using actor_ref_variant = std::variant<local_actor_ref<Actor>, collocated_actor_ref<Actor>>;

    class actor_directory {
    public:
        template<typename Actor>
        static actor_ref_variant<Actor> locate(actor_id id) {
            auto shard = id % seastar::smp::count;
            if (shard == seastar::engine().cpu_id()) {
                return local_actor_ref<Actor>(id);
            } else {
                return collocated_actor_ref<Actor>(id, shard);
            }
        }
    };

    template<typename Actor>
    struct actor_ref {
        actor_ref_variant<Actor> impl;

        explicit actor_ref(actor_id id) : impl(actor_directory::locate<Actor>(id)) {}
        explicit actor_ref(local_actor_ref<Actor> &ref) : impl(ref) {};
        explicit actor_ref(collocated_actor_ref<Actor> &ref) : impl(ref) {};

        actor_ref(actor_ref const &) = default;

        actor_ref(actor_ref &&) noexcept = default;

        template<typename Func>
        auto visit(Func &&func) const {
            return std::visit([func = std::move(func)](auto &impl) {
                return func(impl);
            }, impl);
        }
    };

    template<typename Actor>
    inline actor_ref<Actor> get(actor_id id) {
        return actor_ref<Actor>(id);
    }
}
