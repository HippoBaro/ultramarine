---
layout: default
parent: API index
---

# ultramarine/actor_ref.hpp

``` cpp
#include "actor.hpp"

namespace ultramarine
{
    template <ultramarine::ActorKind = actor_kind<Actor>()>
    class actor_ref;

    template <typename Actor>
    class actor_ref<Actor, ActorKind::SingletonActor>;

    template <typename Actor>
    class actor_ref<Actor, ActorKind::LocalActor>;

    template <typename Actor, typename KeyType = typename Actor::Keytype>
    constexpr actor_ref<Actor> get(KeyType&& key) noexcept;
}
```

## Class `ultramarine::actor_ref`

``` cpp
template <ultramarine::ActorKind = actor_kind<Actor>()>
class actor_ref
{
};
```

A movable and copyable reference to a virtual actor

-----

## Class `ultramarine::actor_ref`

``` cpp
template <typename Actor>
class actor_ref<Actor, ActorKind::SingletonActor>
{
public:
    template <typename KeyType>
    constexpr actor_ref(KeyType&& key);

    constexpr actor_ref(impl::local_actor_ref<Actor> const& ref);

    constexpr actor_ref(impl::collocated_actor_ref<Actor> const& ref);

    constexpr actor_ref(actor_ref<type-parameter-0-0, ultramarine::ActorKind::SingletonActor> const&) = default;

    constexpr actor_ref(actor_ref<type-parameter-0-0, ultramarine::ActorKind::SingletonActor>&&) noexcept = default;

    template <typename Func>
    constexpr auto visit(Func&& func) const noexcept;

    template <typename Handler, typename ... Args>
    constexpr auto tell(Handler message, Args &&... args) const;

    template <typename Handler>
    constexpr auto tell(Handler message) const;
};
```

A movable and copyable reference to a virtual actor

## Template parameter `ultramarine::actor_ref::Actor`

``` cpp
typename Actor
```

The type of actor to reference

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

-----

## Function `ultramarine::actor_ref::visit`

``` cpp
template <typename Func>
constexpr auto visit(Func&& func) const noexcept;
```

Obtain the concrete actor\_ref implementation

## Parameter `ultramarine::actor_ref::func`

``` cpp
Func&& func
```

A lambda to execute with the actor\_ref implementation

*Returns:* The value returned by the provided lambda, if any

-----

## Function `ultramarine::actor_ref::tell`

``` cpp
template <typename Handler, typename ... Args>
constexpr auto tell(Handler message, Args &&... args) const;
```

Enqueue a message to the actor referenced by this actor\_ref instance

*Effects:* Creates the actor if it doesn’t exist

## Parameter `ultramarine::actor_ref::message`

``` cpp
Handler message
```

The message handler to enqueue

*Returns:* A future representing the eventually returned value by the actor, or a failed future

-----

## Function `ultramarine::actor_ref::tell`

``` cpp
template <typename Handler>
constexpr auto tell(Handler message) const;
```

Enqueue a message to the actor referenced by this actor\_ref instance

*Effects:* Creates the actor if it doesn’t exist

## Parameter `ultramarine::actor_ref::message`

``` cpp
Handler message
```

The message handler to enqueue

*Returns:* A future representing the eventually returned value by the actor, or a failed future

-----

## Class `ultramarine::actor_ref`

``` cpp
template <typename Actor>
class actor_ref<Actor, ActorKind::LocalActor>
{
public:
    template <typename KeyType>
    constexpr actor_ref(KeyType&& key);

    constexpr actor_ref(actor_ref<type-parameter-0-0, ultramarine::ActorKind::LocalActor> const&) = default;

    constexpr actor_ref(actor_ref<type-parameter-0-0, ultramarine::ActorKind::LocalActor>&&) noexcept = default;

    template <typename Func>
    constexpr auto visit(Func&& func) const noexcept;

    template <typename Handler, typename ... Args>
    constexpr auto tell(Handler message, Args &&... args) const;

    template <typename Handler>
    constexpr auto tell(Handler message) const;
};
```

A movable and copyable reference to a virtual actor

## Template parameter `ultramarine::actor_ref::Actor`

``` cpp
typename Actor
```

The type of actor to reference

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) and from attribute [`ultramarine::local_actor`](doc_ultramarine__actor_attributes.md#standardese-ultramarine__local_actor)

-----

## Function `ultramarine::actor_ref::visit`

``` cpp
template <typename Func>
constexpr auto visit(Func&& func) const noexcept;
```

Obtain the concrete actor\_ref implementation

## Parameter `ultramarine::actor_ref::func`

``` cpp
Func&& func
```

A lambda to execute with the actor\_ref implementation

*Returns:* The value returned by the provided lambda, if any

-----

## Function `ultramarine::actor_ref::tell`

``` cpp
template <typename Handler, typename ... Args>
constexpr auto tell(Handler message, Args &&... args) const;
```

Enqueue a message to the actor referenced by this actor\_ref instance

*Effects:* Creates the actor if it doesn’t exist

### Parameters

  - `message` - The message handler to enqueue

## Parameter `ultramarine::actor_ref::args`

``` cpp
Args &&... args
```

Arguments to pass to the message handler

*Returns:* A future representing the eventually returned value by the actor, or a failed future

-----

## Function `ultramarine::actor_ref::tell`

``` cpp
template <typename Handler>
constexpr auto tell(Handler message) const;
```

Enqueue a message to the actor referenced by this actor\_ref instance

*Effects:* Creates the actor if it doesn’t exist

## Parameter `ultramarine::actor_ref::message`

``` cpp
Handler message
```

The message handler to enqueue

*Returns:* A future representing the eventually returned value by the actor, or a failed future

-----

## Function `ultramarine::get`

``` cpp
template <typename Actor, typename KeyType = typename Actor::Keytype>
constexpr actor_ref<Actor> get(KeyType&& key) noexcept;
```

Create a reference to a virtual actor

## Template parameter `ultramarine::Actor`

``` cpp
typename Actor
```

The type of actor to reference

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

-----

## Parameter `ultramarine::key`

``` cpp
KeyType&& key
```

The primary key of the actor

*Returns:* A [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-)

-----
