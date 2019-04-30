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

*Requires:* `name` shall be a [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) derived type

*Requires:* `seq` shall be a sequence of zero or more message handler (Example: `(handler1)(handler2)`)

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
