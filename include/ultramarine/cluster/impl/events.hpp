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

#include <functional>
#include <vector>
#include <seastar/core/weak_ptr.hh>
#include "node.hpp"


namespace ultramarine::cluster {
    template<typename ...Args>
    struct event_listener : seastar::weakly_referencable<event_listener<Args...>> {
        std::optional<std::function<seastar::future<>(Args...)>> callback;

        virtual ~event_listener() = default;

        template<typename Func>
        void set_callback(Func &&c) {
            callback = std::forward<Func>(c);
        }

        event_listener() = default;

        event_listener(event_listener const &) = delete;
    };

    template<typename ...Args>
    struct event_emitter {
        std::vector<seastar::weak_ptr<event_listener<Args...>>> listeners;

        event_emitter() = default;

        event_emitter(event_emitter const &) = delete;

        std::unique_ptr<event_listener<Args...>> make_listener() {
            auto listener = std::make_unique<event_listener<Args...>>();
            listeners.emplace_back(listener->weak_from_this());
            return listener;
        }

        template<typename ...TArgs>
        seastar::future<> raise(TArgs &&...args) const {
            return seastar::parallel_for_each(listeners, [this, args = std::make_tuple(std::forward<TArgs>(args) ...)](
                    auto const &listener) {
                if (listener && listener->callback) {
                    return std::apply([&listener](Args... args) {
                        return (*listener->callback)(args...);
                    }, std::move(args));
                }
                return seastar::make_ready_future();
            });
        }
    };
}
