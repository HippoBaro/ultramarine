---
title: Actor identity
layout: default
parent: Concepts
---

# Actor identity

In contrast with typical object-oriented programming, actors in Ultramarine are *transcendental*, they exist all the time, *somehow*.
Actors, therefore, need to be identifiable regardless of their concrete existence in memory.

In Ultramarine, actors are identified using a **key**. By default, actors have a `std::size_t` key type (see [ultramarine::actor::KeyType](../api/doc_ultramarine__actor.md#standardese-ultramarine__actor__KeyType)).

## Attributing a key type to an actor

You can change an actor key type in the actor definition by declaring a `typdef` for the alias `KeyType`:

```cpp
class string_actor : public ultramarine::actor<string_actor> {
public:
    using KeyType = std::string;

    ULTRAMARINE_DEFINE_ACTOR(string_actor,);
};
```

## Retrieving an actor's key

Each actor instance will contain a read-only copy of it's own key, in this case, a `std::string`. You can access it via `this->key`:

```cpp
class string_actor : public ultramarine::actor<string_actor> {
public:
    using KeyType = std::string;
    
    seastar::future<> say_hello() const {
        seastar::print("Hello, World; from string_actor '%s'.\n", key);
        return seastar::make_ready_future();
    }

    ULTRAMARINE_DEFINE_ACTOR(string_actor, (say_hello));
};
```

Note that Ultramarine's actors don't have any internal state except for the actor's key; therefore our `string_actor` has a `sizeof` of `sizeof(std::string)`.

## Using custom key types

Any type can be used as an actor key as long as they are [`Moveable`](https://en.cppreference.com/w/cpp/concepts/Moveable), [`Copyable`](https://en.cppreference.com/w/cpp/concepts/Copyable) and hashable using [`std::hash`](https://en.cppreference.com/w/cpp/utility/hash). Here is an example:

```cpp
// Our key type
struct custom_key {
    int inner_key;
};

// We provide a std::hash specialisation
namespace std {
    template <> struct hash<custom_key> {
        std::size_t operator()(const custom_key& k) const {
            return k.inner_key;
        }
    };
}

class custom_key_actor : public ultramarine::actor<custom_key_actor> {
public:
    using KeyType = custom_key;

    ULTRAMARINE_DEFINE_ACTOR(custom_key_actor,);
};
```
