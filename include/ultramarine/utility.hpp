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

#include <utility>
#include <boost/circular_buffer.hpp>
#include <seastar/core/reactor.hh>

namespace ultramarine {
    /// A dynamic message buffer storing a set number of futures running concurrently
    /// \tparam Future The future type the message buffer will store
    /// \requires Type `Future` shall be a `seastar::future`
    /// \unique_name ultramarine::message_buffer
    template<typename Future>
    struct message_buffer {
        boost::circular_buffer<Future> futs;

        explicit message_buffer(std::size_t capacity) : futs(capacity) {};

        message_buffer(message_buffer const &) = delete;

        message_buffer(message_buffer &&) noexcept = default;

        /// Push a future in the message buffer
        /// \param fut The future to push in the message buffer
        /// \returns An immediately available future, or a unresolved future if the message buffer is full
        auto operator() (Future &&fut) {
            auto ret = seastar::make_ready_future();
            if (futs.full() && !(futs.front().available() || futs.front().failed())) {
                ret = std::move(futs.front());
            }
            futs.push_back(std::move(fut));
            return ret;
        }

        /// Flush the message buffer such that all future it contains are in an available or failed sate
        /// \returns A future
        auto flush() {
            return seastar::when_all(std::begin(futs), std::end(futs)).discard_result().then([this] {
                futs.clear();
            });
        }
    };

    /// Create a [ultramarine::message_buffer]() to use in a specified function scope
    /// \param capacity The number of message the buffer should be able to store before awaiting
    /// \param func A lambda function using the message buffer
    /// \returns Any value returned by the provided lambda function
    template<typename Func>
    auto with_buffer(std::size_t capacity, Func &&func) {
        return seastar::do_with(message_buffer<seastar::future<>>(capacity), [func = std::forward<Func>(func)] (auto &buff) {
            return func(buff).then([&buff] { return buff.flush(); });
        });
    }
};
