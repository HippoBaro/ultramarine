---
layout: default
parent: API index
---

# ultramarine/directory.hpp

``` cpp
namespace ultramarine
{
    using actor_id = std::size_t;

    struct round_robin_local_placement_strategy;

    using default_local_placement_strategy = ultramarine::round_robin_local_placement_strategy;
}
```

## Type alias `ultramarine::actor_id`

``` cpp
using actor_id = std::size_t;
```

Actors are fetched internally via an unsigned integer id

-----

## Struct `ultramarine::round_robin_local_placement_strategy`

``` cpp
struct round_robin_local_placement_strategy
{
    seastar::shard_id operator()(std::size_t hash) const noexcept;
};
```

A round-robin placement strategy that shards actors based on the modulo of their hashed key

-----

## Type alias `ultramarine::default_local_placement_strategy`

``` cpp
using default_local_placement_strategy = ultramarine::round_robin_local_placement_strategy;
```

Default local placement strategy uses round\_robin\_local\_placement\_strategy

-----
