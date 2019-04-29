---
layout: default
parent: API index
---

# ultramarine/actor_attributes.hpp

``` cpp
namespace ultramarine
{
    template <typename Derived, std::size_t ConcurrencyLimit = std::numeric_limits<std::size_t>::max()>
    struct local_actor;

    template <typename Derived>
    struct non_reentrant_actor;
}
```

## Struct `ultramarine::local_actor`

``` cpp
template <typename Derived, std::size_t ConcurrencyLimit = std::numeric_limits<std::size_t>::max()>
struct local_actor
{
    static_assert(ConcurrencyLimit>0, "Local actor concurrency limit must be a positive integer");
};
```

Actor attribute base class that specify that the Derived actor should be treated as a local actor

### Template parameters

  - `Derived` - The derived actor class for CRTP purposes
  - `ConcurrencyLimit` - Optional. The limit of concurrent local activations for this actor

-----

## Struct `ultramarine::non_reentrant_actor`

``` cpp
template <typename Derived>
struct non_reentrant_actor
{
};
```

Actor attribute base class that specify that the Derived actor should be protected against reentrancy

*Requires:* Type `Derived` should be of type [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)

### Template parameters

  - `Derived` - The derived actor class for CRTP purposes

-----
