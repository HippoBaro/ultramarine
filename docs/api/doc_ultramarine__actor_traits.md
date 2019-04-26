---
layout: default
parent: API index
---

# ultramarine/actor_traits.hpp

``` cpp
#include "actor_attributes.hpp"

namespace ultramarine
{
    enum class ActorKind;

    template <typename Actor>
    constexpr ultramarine::ActorKind actor_kind();

    template <typename Actor>constexpr bool is_reentrant_v = !std::is_base_of_v<non_reentrant_actor<Actor>, Actor>;

    template <typename Actor>constexpr bool is_local_actor_v = std::is_base_of_v<impl::local_actor, Actor>;

    template <typename Actor>constexpr bool is_unlimited_concurrent_local_actor_v = std::is_base_of_v<local_actor<Actor>, Actor>;
}
```

### Enumeration `ultramarine::ActorKind`

``` cpp
enum class ActorKind
{
    SingletonActor,
    LocalActor
};
```

Enum representing the possible kinds of actor

-----

### Function `ultramarine::actor_kind`

``` cpp
template <typename Actor>
constexpr ultramarine::ActorKind actor_kind();
```

Get the actor type

### Template parameter `ultramarine::Actor`

``` cpp
typename Actor
```

The actor type to test against

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

*Returns:* An enum value of type [`ultramarine::actor_type`](doc_ultramarine__actor_traits.md#standardese-ultramarine__actor_type)

-----

-----

### Unexposed entity `ultramarine::is_reentrant_v`

``` cpp
template <typename Actor>constexpr bool is_reentrant_v = !std::is_base_of_v<non_reentrant_actor<Actor>, Actor>;
```

Compile-time trait returning true if the actor type is reentrant

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

-----

### Unexposed entity `ultramarine::is_local_actor_v`

``` cpp
template <typename Actor>constexpr bool is_local_actor_v = std::is_base_of_v<impl::local_actor, Actor>;
```

Compile-time trait returning true if actor type is local

*Requires:* Type `Actor` shall inherit from [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

-----

### Unexposed entity `ultramarine::is_unlimited_concurrent_local_actor_v`

``` cpp
template <typename Actor>constexpr bool is_unlimited_concurrent_local_actor_v = std::is_base_of_v<local_actor<Actor>, Actor>;
```

Compile-time trait returning true if the local actor type doesn’t specify a concurrency limit

*Requires:* Type `Actor` shall inherit from [`ultramarine::local_actor`](doc_ultramarine__actor_attributes.md#standardese-ultramarine__local_actor)

-----
