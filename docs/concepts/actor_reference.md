---
title: Actor references
layout: default
parent: Concepts
---

# Actor references

Like with [identity](actor_identity.md), actors provide a way to be referenced regardless of their concrete existence in
memory.

Ultramarine provide opaque objects called [actor_ref](../api/doc_ultramarine__actor_ref.md) that allow user-code to
interact with actors, just like pointers. Actor references can be freely created, moved, copied and sent as message
arguments.

## Referencing an actor

All actor references are created using an [identity key](actor_identity.md), unique to each actor. Referencing an actor by it's key in code:

```cpp
ultramarine::actor_ref<example> ref = ultramarine::get<example>(my_key);
```

Creating a reference to an actor *doesn't allocate resources*. Furthermore, holding a reference to an actor does not
mean anything concerning the current state of the referenced actor.

## Scheduling a message

Actor references provide an intuitive way of scheduling a message to an actor, via
[`operator->`](../api/doc_ultramarine__actor_ref.md):

```cpp
auto future = ref->my_message(my_argument);
...
```

Alternatively, a more verbose syntax is also available, using the struct `message` created by the [`ULTRAMARINE_DEFINE_ACTOR`](../api/doc_ultramarine__macro.md) macro:

```cpp
auto future = ref.tell(example::message::my_message, my_argument);
...
```