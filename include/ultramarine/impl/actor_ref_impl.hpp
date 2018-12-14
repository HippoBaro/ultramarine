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

#include "ultramarine/actor.hpp"
#include "directory.hpp"

namespace ultramarine::impl {
    template<typename Impl>
    struct actor_ref_impl {
        const std::size_t hash;

        explicit constexpr actor_ref_impl(std::size_t hash) : hash(hash) {}

        template<typename Handler, typename ...Args>
        constexpr auto tell(Handler message, Args &&... args) const {
            using Actor = typename Impl::ActorType;
            using Return = std::result_of_t<decltype(vtable<Actor>::table[message])(Actor, Args &&...)>;

            return [this, message, args = std::make_tuple(std::forward<Args>(args) ...)]() mutable {
                if constexpr (std::is_void<Return>::value) {
                    return seastar::futurize<Return>::apply([this, message, &args] {
                        return std::apply([this, message](auto &&... args) {
                            static_cast<Impl const *>(this)->schedule(message, std::forward<Args>(args) ...);
                        }, std::move(args));
                    });
                } else if constexpr (!seastar::is_future<Return>::value) {
                    return seastar::futurize<Return>::apply([this, message, &args]() mutable {
                        return std::apply([this, message](auto &&... args) {
                            return static_cast<Impl const *>(this)->schedule(message, std::forward<Args>(args) ...);
                        }, std::move(args));
                    });
                } else {
                    return std::apply([this, message](auto &&... args) {
                        return static_cast<Impl const *>(this)->schedule(message, std::forward<Args>(args) ...);
                    }, std::move(args));
                }
            }();
        }

        template<typename Handler>
        constexpr auto tell(Handler message) const {
            using Actor = typename Impl::ActorType;
            using Return = std::result_of_t<decltype(vtable<Actor>::table[message])(Actor)>;

            if constexpr (std::is_void<Return>::value) {
                return seastar::futurize<Return>::apply([this, message] {
                    static_cast<Impl const *>(this)->schedule(message);
                });
            } else if constexpr (!seastar::is_future<Return>::value) {
                return seastar::futurize<Return>::apply([this, message] {
                    return static_cast<Impl const *>(this)->schedule(message);
                });
            } else {
                return static_cast<Impl const *>(this)->schedule(message);
            }
        };
    };

    template<typename Actor>
    class collocated_actor_ref : public actor_ref_impl<collocated_actor_ref<Actor>> {
        ActorKey <Actor> key;
        seastar::shard_id loc;

    public:
        using ActorType = Actor;

        template<typename KeyType>
        explicit constexpr collocated_actor_ref(KeyType &&k, std::size_t hash, seastar::shard_id loc):
                actor_ref_impl<collocated_actor_ref<Actor>>(hash), key(std::forward<KeyType>(k)), loc(loc) {}

        using actor_ref_impl<collocated_actor_ref>::tell;

        template<typename Handler>
        inline constexpr auto schedule(Handler message) const {
            return seastar::smp::submit_to(loc, [k = this->key, h = this->hash, message] {
                return (actor_directory<Actor>::hold_activation(k, h)->*vtable<Actor>::table[message])();
            });
        }

        template<typename Handler, typename ...Args>
        inline constexpr auto schedule(Handler message, Args &&... args) const {
            return seastar::smp::submit_to(loc, [k = this->key, h = this->hash, message, args = std::make_tuple(
                    std::forward<Args>(args) ...)]() mutable {
                return std::apply([&k, h, message](auto &&... args) {
                    return (actor_directory<Actor>::hold_activation(k, h)->*vtable<Actor>::table[message])(std::forward<Args>(args) ...);
                }, std::move(args));
            });
        }
    };

    template<typename Actor>
    class local_actor_ref : public actor_ref_impl<local_actor_ref<Actor>> {
        ActorKey <Actor> key;
        Actor *inst = nullptr;

    public:
        using ActorType = Actor;

        template<typename KeyType>
        explicit constexpr local_actor_ref(KeyType &&k, std::size_t hash) :
                actor_ref_impl<local_actor_ref<Actor>>{hash}, key(std::forward<KeyType>(k)),
                inst(actor_directory<Actor>::hold_activation(key, hash)) {};

        using actor_ref_impl<local_actor_ref<Actor>>::tell;

        template<typename Handler>
        inline constexpr auto schedule(Handler message) const {
            return (inst->*vtable<Actor>::table[message])();
        }

        template<typename Handler, typename ...Args>
        inline constexpr auto schedule(Handler message, Args &&... args) const {
            return (inst->*vtable<Actor>::table[message])(std::forward<Args>(args) ...);
        }
    };

    template<typename Actor>
    using actor_ref_variant = std::variant<local_actor_ref<Actor>, collocated_actor_ref<Actor>>;

    template<typename Actor, typename KeyType>
    [[nodiscard]] static constexpr actor_ref_variant<Actor> make_actor_ref_impl(KeyType &&key) noexcept {
        auto hash = actor_directory<Actor>::hash_key(std::forward<KeyType>(key));
        auto shard = hash % seastar::smp::count;
        if (shard == seastar::engine().cpu_id()) {
            return local_actor_ref<Actor>(std::forward<KeyType>(key), hash);
        } else {
            return collocated_actor_ref<Actor>(std::forward<KeyType>(key), hash, shard);
        }
    }
}