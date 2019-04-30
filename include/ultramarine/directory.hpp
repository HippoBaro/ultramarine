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

#include <seastar/core/reactor.hh>

namespace ultramarine {

    /// [ultramarine::actor]() are identified internally via an unsigned integer id
    /// \unique_name ultramarine::actor_id
    using actor_id = std::size_t;

    /// A round-robin placement strategy that shards actors based on the modulo of their [ultramarine::actor::KeyType]()
    /// \unique_name ultramarine::round_robin_local_placement_strategy
    struct round_robin_local_placement_strategy {
        /// \param A hashed [ultramarine::actor::KeyType]()
        /// \returns The location the actor should be placed in
        seastar::shard_id operator()(std::size_t hash) const noexcept {
            return hash % seastar::smp::count;
        }
    };

    /// Default local placement strategy uses [ultramarine::round_robin_local_placement_strategy]()
    /// \unique_name ultramarine::default_local_placement_strategy
    using default_local_placement_strategy = round_robin_local_placement_strategy;
}