---
layout: default
parent: API index
---

# ultramarine/macro.hpp

``` cpp
#define ULTRAMARINE_DEFINE_ACTOR(name, seq)
```

# Macro `ULTRAMARINE_DEFINE_ACTOR`

``` cpp
#define ULTRAMARINE_DEFINE_ACTOR(name, seq)
```

Expands with enclosing actor internal definitions

Example:

``` cpp
class simple_actor : public ultramarine::actor<simple_actor> {
public:
    seastar::future<> my_message() const;
    seastar::future<> another_message() const;

    ULTRAMARINE_DEFINE_ACTOR(simple_actor, (my_message)(another_message));
};
```

-----
