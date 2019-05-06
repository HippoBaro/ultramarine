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

#include <ultramarine/actor_ref.hpp>
#include "global_message_handlers.hpp"

namespace ultramarine::cluster {
    template<typename Actor, typename ActorKey, typename Ret, typename Class, typename ...Args, typename Handler>
    static constexpr void __attribute__ ((used))
    register_remote_endpoint(Ret (Class::*fptr)(Args...), Handler message) {
        auto reg = [fptr, message](auto *rpc) {
            rpc->register_handler(message.value, [message](ActorKey key, Args... args) {
                return ultramarine::get<Actor>(std::forward<ActorKey>(key)).tell(message, std::forward<Args>(args)...);
            });
        };
        impl::remote_init_handlers().insert({message.value, reg});
    }

    template<typename Actor, typename ActorKey, typename Ret, typename Class, typename ...Args, typename Handler>
    static constexpr void __attribute__ ((used))
    register_remote_endpoint(Ret (Class::*fptr)(Args...) const, Handler message) {
        auto reg = [fptr, message](auto *rpc) {
            rpc->register_handler(message.value, [message](ActorKey key, Args... args) {
                return ultramarine::get<Actor>(std::forward<ActorKey>(key)).tell(message, std::forward<Args>(args)...);
            });
        };
        impl::remote_init_handlers().insert({message.value, reg});
    }
}