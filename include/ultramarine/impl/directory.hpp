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

namespace ultramarine::impl {
    template<typename Actor>
    struct actor_directory {

        template<typename KeyType>
        [[nodiscard]] static constexpr auto hash_key(KeyType &&key) noexcept {
            return std::hash<ActorKey<Actor>>{}(key);
        }

        [[nodiscard]] static constexpr Actor *hold_activation(ActorKey <Actor> const &key, actor_id id) {
            if (!Actor::directory) [[unlikely]] {
                Actor::directory = std::make_unique<ultramarine::directory<Actor>>();
            }

            auto r = std::get<0>(Actor::directory->try_emplace(id, key));
            return &(std::get<1>(*r));
        }

        template<typename Handler, typename ...Args>
        static constexpr auto dispatch_message(Actor *activation, Handler message, Args &&... args) {
            if constexpr (!is_reentrant_v < Actor >) {
                return seastar::with_semaphore(activation->semaphore, 1, std::chrono::seconds(1),
                                               [message, activation, args = std::make_tuple(
                                                       std::forward<Args>(args) ...)] {
                                                   return std::apply([activation, message](auto &&... args) {
                                                       return (activation->*vtable<Actor>::table[message])(
                                                               std::forward<Args>(args) ...);
                                                   }, std::move(args));
                                               });
            }
            return (activation->*vtable<Actor>::table[message])(std::forward<Args>(args) ...);
        }

        template<typename Handler>
        static constexpr auto dispatch_message(Actor *activation, Handler message) {
            if constexpr (!is_reentrant_v < Actor >) {
                return seastar::with_semaphore(activation->semaphore, 1, std::chrono::seconds(1),
                                               [message, activation] {
                                                   return (activation->*vtable<Actor>::table[message])();
                                               });
            }
            return (activation->*vtable<Actor>::table[message])();
        }

        template<typename KeyType, typename Handler, typename ...Args>
        static constexpr auto dispatch_message(KeyType &&key, actor_id id, Handler message, Args &&... args) {
            return dispatch_message(hold_activation(key, id), message, std::forward<Args>(args) ...);
        }

        template<typename KeyType, typename Handler>
        static constexpr auto dispatch_message(KeyType &&key, actor_id id, Handler message) {
            return dispatch_message(hold_activation(key, id), message);
        }
    };
}
