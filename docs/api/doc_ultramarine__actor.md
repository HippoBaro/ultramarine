---
layout: default
parent: API index
---

# ultramarine/actor.hpp

``` cpp
namespace ultramarine
{
    template <typename Derived, typename LocalPlacementStrategy = default_local_placement_strategy>
    struct actor;
}
```

## Struct `ultramarine::actor`

``` cpp
template <typename Derived, typename LocalPlacementStrategy = default_local_placement_strategy>
struct actor
{
    using KeyType = ultramarine::actor_id;

    using PlacementStrategy = LocalPlacementStrategy;

    static seastar::future<> clear_directory();
};
```

Base template class defining an actor

### Template parameters

  - `Derived` - The derived actor class for CRTP purposes

## Template parameter `ultramarine::actor::LocalPlacementStrategy`

``` cpp
typename LocalPlacementStrategy = default_local_placement_strategy
```

Optional. Allows to specify a custom local placement strategy. Defaults to [`ultramarine::default_local_placement_strategy`](doc_ultramarine__directory.md#standardese-ultramarine__default_local_placement_strategy)

*Requires:* `Derived` should implement actor behavior using [`ULTRAMARINE_DEFINE_ACTOR`](doc_ultramarine__macro.md#standardese-ULTRAMARINE_DEFINE_ACTOR)

-----

## Type alias `ultramarine::actor::KeyType`

``` cpp
using KeyType = ultramarine::actor_id;
```

Default key type (unsigned long integer)

See [`ultramarine::actor_id`](doc_ultramarine__directory.md#standardese-ultramarine__actor_id)

-----

## Type alias `ultramarine::actor::PlacementStrategy`

``` cpp
using PlacementStrategy = LocalPlacementStrategy;
```

Default placement strategy

-----

## Function `ultramarine::actor::clear_directory`

``` cpp
static seastar::future<> clear_directory();
```

*Effects:* Clears all actors of type Derived in all shards

*Returns:* A future available when all instances of this actor type have been purged

-----
