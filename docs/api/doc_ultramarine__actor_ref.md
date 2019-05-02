---
layout: default
parent: API index
---

# ultramarine/actor_ref.hpp

``` cpp
namespace ultramarine
{
    template <ultramarine::ActorKind = actor_kind<Actor>()>
    class actor_ref;

    template <typename Actor>
    class actor_ref<Actor, ActorKind::SingletonActor>;

    template <typename Actor>
    class actor_ref<Actor, ActorKind::LocalActor>;

    class poly_actor_ref;

    template <typename Actor, typename KeyType = typename Actor::KeyType>
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
    using ActorType = Actor;

    template <typename KeyType>
    constexpr actor_ref(KeyType key);

    constexpr actor_ref(actor_ref<type-parameter-0-0, ultramarine::ActorKind::SingletonActor> const&) = default;

    constexpr actor_ref(actor_ref<type-parameter-0-0, ultramarine::ActorKind::SingletonActor>&&) noexcept = default;

    template <typename Func>
    constexpr auto visit(Func&& func) const noexcept;

    constexpr typename Actor::message::interface<actor_ref<Actor> > operator->() const;

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

The type of [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) to reference

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

-----

## Function `ultramarine::actor_ref::visit`

``` cpp
template <typename Func>
constexpr auto visit(Func&& func) const noexcept;
```

Obtain the concrete [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) implementation

## Parameter `ultramarine::actor_ref::func`

``` cpp
Func&& func
```

A lambda to execute with the [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) implementation

*Returns:* The value returned by the provided lambda, if any

-----

## Function `ultramarine::actor_ref::operator->`

``` cpp
constexpr typename Actor::message::interface<actor_ref<Actor> > operator->() const;
```

Provides an intuitive function call-like API.

*Returns:* Returns the remote actor’s interface

The syntax `ref->msg(args...)` is equivalent to `ref.tell(actor::message::msg, args...)` but shorter.

-----

## Function `ultramarine::actor_ref::tell`

``` cpp
template <typename Handler, typename ... Args>
constexpr auto tell(Handler message, Args &&... args) const;
```

Enqueue a message to the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) referenced by this [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) instance

*Effects:* Creates the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) if it doesn’t exist

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

Enqueue a message to the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) referenced by this [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) instance

*Effects:* Creates the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) if it doesn’t exist

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
    using ActorType = Actor;

    template <typename KeyType>
    constexpr actor_ref(KeyType key);

    constexpr actor_ref(actor_ref<type-parameter-0-0, ultramarine::ActorKind::LocalActor> const&) = default;

    constexpr actor_ref(actor_ref<type-parameter-0-0, ultramarine::ActorKind::LocalActor>&&) noexcept = default;

    template <typename Func>
    constexpr auto visit(Func&& func) const noexcept;

    constexpr typename Actor::message::interface<actor_ref<Actor> > operator->() const;

    template <typename Handler, typename ... Args>
    constexpr auto tell(Handler message, Args &&... args) const;

    template <typename Handler>
    constexpr auto tell(Handler message) const;
};
```

A movable and copyable reference to an [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

## Template parameter `ultramarine::actor_ref::Actor`

``` cpp
typename Actor
```

The type of [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) to reference

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) and from attribute [`ultramarine::local_actor`](doc_ultramarine__actor_attributes.md#standardese-ultramarine__local_actor)

-----

## Function `ultramarine::actor_ref::visit`

``` cpp
template <typename Func>
constexpr auto visit(Func&& func) const noexcept;
```

Obtain the concrete [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) implementation

## Parameter `ultramarine::actor_ref::func`

``` cpp
Func&& func
```

A lambda to execute with the [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) implementation

*Returns:* The value returned by the provided lambda, if any

-----

## Function `ultramarine::actor_ref::operator->`

``` cpp
constexpr typename Actor::message::interface<actor_ref<Actor> > operator->() const;
```

Provides an intuitive function call-like API.

*Returns:* Returns the remote actor’s interface

The syntax `ref->msg(args...)` is equivalent to `ref.tell(actor::message::msg, args...)` but shorter.

-----

## Function `ultramarine::actor_ref::tell`

``` cpp
template <typename Handler, typename ... Args>
constexpr auto tell(Handler message, Args &&... args) const;
```

Enqueue a message to the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) referenced by this [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) instance

*Effects:* Creates the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) if it doesn’t exist

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

Enqueue a message to the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) referenced by this [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) instance

*Effects:* Creates the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) if it doesn’t exist

## Parameter `ultramarine::actor_ref::message`

``` cpp
Handler message
```

The message handler to enqueue

*Returns:* A future representing the eventually returned value by the actor, or a failed future

-----

## Class `ultramarine::poly_actor_ref`

``` cpp
class poly_actor_ref
{
public:
    template <template <typename> class Ref, typename Actor>
    constexpr poly_actor_ref(Ref<Actor>&& ref) noexcept;

    template <template <typename> class Ref, typename Actor>
    constexpr poly_actor_ref(const Ref<Actor>& ref) noexcept;

    template <typename Actor>
    auto as();
};
```

A movable and copyable type-erased reference to a virtual actor.

*Remarks:* Use only when needed, as this type introduces overhead

Useful when an [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) declares a message with an `actor_ref<itself>` as argument. Avoids incomplete type compiler error.

## Function `ultramarine::poly_actor_ref::as`

``` cpp
template <typename Actor>
auto as();
```

Cast this instance into a fully specified actor\_ref

*Requires:* Actor shall be of type [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

## Template parameter `ultramarine::poly_actor_ref::Actor`

``` cpp
typename Actor
```

The type of [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) to reference

*Returns:* An [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-)

-----

## Function `ultramarine::get`

``` cpp
template <typename Actor, typename KeyType = typename Actor::KeyType>
constexpr actor_ref<Actor> get(KeyType&& key) noexcept;
```

Create a reference to a virtual actor

## Template parameter `ultramarine::Actor`

``` cpp
typename Actor
```

The type of [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) to reference

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

*Requires:* Type `KeyType` shall be of type `Actor::KeyType`

-----

## Parameter `ultramarine::key`

``` cpp
KeyType&& key
```

The primary key of the actor

*Returns:* An [`ultramarine::actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-)

-----
