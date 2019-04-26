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

#include <seastar/core/future.hh>
#include <seastar/core/reactor.hh>
#include "directory.hpp"

namespace ultramarine::impl {
    template<typename Actor>
    class collocated_actor_ref {
        const ActorKey<Actor> key;
        const std::size_t hash;
        const seastar::shard_id loc;

    public:
        using ActorType = Actor;

        template<typename KeyType>
        explicit constexpr collocated_actor_ref(KeyType &&k, std::size_t hash, seastar::shard_id loc):
                key(std::forward<KeyType>(k)), hash(hash), loc(loc) {}

        template<typename Handler>
        inline constexpr auto tell(Handler message) const {
            return seastar::smp::submit_to(loc, [k = this->key, h = this->hash, message] {
                return actor_directory<Actor>::dispatch_message(k, h, message);
            });
        }

        template<typename Handler, typename ...Args>
        inline constexpr auto tell(Handler message, Args &&... args) const {
            return seastar::smp::submit_to(loc, [k = this->key, h = this->hash, message, args = std::make_tuple(
                    std::forward<Args>(args) ...)]() mutable {
                return std::apply([&k, h, message](auto &&... args) {
                    return actor_directory<Actor>::dispatch_message(k, h, message, std::forward<Args>(args) ...);
                }, std::move(args));
            });
        }
    };

    template<typename Actor>
    class local_actor_ref {
        const ActorKey<Actor> key;
        const std::size_t hash;
        const Actor *inst = nullptr;

    public:
        using ActorType = Actor;

        template<typename KeyType>
        explicit constexpr local_actor_ref(KeyType &&k, std::size_t hash) :
                key(std::forward<KeyType>(k)), hash(hash), inst(actor_directory<Actor>::hold_activation(key, hash)) {};

        template<typename Handler>
        inline constexpr auto tell(Handler message) const {
            const auto cpu = seastar::engine().cpu_id();
            return seastar::smp::submit_to(cpu, [k = this->key, h = this->hash, message] {
                return actor_directory<Actor>::dispatch_message(k, h, message);
            });
        }

        template<typename Handler, typename ...Args>
        inline constexpr auto tell(Handler message, Args &&... args) const {
            const auto cpu = seastar::engine().cpu_id();
            return seastar::smp::submit_to(cpu, [k = this->key, h = this->hash, message, args = std::make_tuple(
                    std::forward<Args>(args) ...)]() mutable {
                return std::apply([&k, h, message](auto &&... args) {
                    return actor_directory<Actor>::dispatch_message(k, h, message, std::forward<Args>(args) ...);
                }, std::move(args));
            });
        }
    };

    template<typename Actor>
    using actor_ref_variant = std::variant<local_actor_ref<Actor>, collocated_actor_ref<Actor>>;

    template<typename Actor, typename KeyType, typename Func>
    [[nodiscard]] constexpr auto do_with_actor_ref_impl(KeyType &&key, Func &&func) noexcept {
        auto hash = actor_directory<Actor>::hash_key(std::forward<KeyType>(key));
        auto shard = typename Actor::PlacementStrategy{}(hash);
        if (shard == seastar::engine().cpu_id()) {
            return func(local_actor_ref<Actor>(std::forward<KeyType>(key), hash));
        }
        return func(collocated_actor_ref<Actor>(std::forward<KeyType>(key), hash, shard));
    }

    template<typename Actor, typename KeyType, typename Func>
    [[nodiscard]] constexpr auto
    do_with_actor_ref_impl(KeyType key, seastar::shard_id shard, Func &&func) noexcept {
        auto hash = actor_directory<Actor>::hash_key(shard);
        if (shard == seastar::engine().cpu_id()) {
            return func(local_actor_ref<Actor>(std::forward<KeyType>(key), hash));
        }
        return func(collocated_actor_ref<Actor>(std::forward<KeyType>(key), hash, shard));
    }

    template<typename Actor, typename KeyType>
    [[nodiscard]] constexpr actor_ref_variant<Actor> wrap_actor_ref_impl(KeyType &&key) {
        return do_with_actor_ref_impl<Actor, KeyType>(std::forward<KeyType>(key), [](auto &&impl) {
            return actor_ref_variant<Actor>(std::forward<decltype(impl)>(impl));
        });
    }
}