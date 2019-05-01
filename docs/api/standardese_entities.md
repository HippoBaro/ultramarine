---
title: API index
layout: default
has_children: true
permalink: /api
nav_order: 99
---

# Project index

  - [`ULTRAMARINE_DEFINE_ACTOR`](doc_ultramarine__macro.md#standardese-ULTRAMARINE_DEFINE_ACTOR) - Expands with enclosing actor internal definitions

  - ## Namespace `ultramarine`
    
      - [`ActorKind`](doc_ultramarine__actor_traits.md#standardese-ultramarine__actor_type) - Enum representing the possible kinds of [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor)
    
      - [`actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) - Base template class defining an actor
    
      - [`actor_id`](doc_ultramarine__directory.md#standardese-ultramarine__actor_id) - [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) are identified internally via an unsigned integer id
    
      - [`actor_kind`](doc_ultramarine__actor_traits.md#standardese-ultramarine__actor_kind-Actor---) - Get the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) type
    
      - [`actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) - A movable and copyable reference to a virtual actor
    
      - [`default_local_placement_strategy`](doc_ultramarine__directory.md#standardese-ultramarine__default_local_placement_strategy) - Default local placement strategy uses [`ultramarine::round_robin_local_placement_strategy`](doc_ultramarine__directory.md#standardese-ultramarine__round_robin_local_placement_strategy)
    
      - [`get`](doc_ultramarine__actor_ref.md#standardese-ultramarine__get-Actor-KeyType--KeyType---) - Create a reference to a virtual actor
    
      - [`is_local_actor_v`](doc_ultramarine__actor_traits.md#standardese-ultramarine__is_local_actor_v) - Compile-time trait testing if the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) type is local
    
      - [`is_reentrant_v`](doc_ultramarine__actor_traits.md#standardese-ultramarine__is_reentrant_v) - Compile-time trait testing if the [`ultramarine::actor`](doc_ultramarine__actor.md#standardese-ultramarine__actor) type is reentrant
    
      - [`is_unlimited_concurrent_local_actor_v`](doc_ultramarine__actor_traits.md#standardese-ultramarine__is_unlimited_concurrent_local_actor_v) - Compile-time trait testing if the [`ultramarine::local_actor`](doc_ultramarine__actor_attributes.md#standardese-ultramarine__local_actor) type doesn’t specify a concurrency limit
    
      - [`local_actor`](doc_ultramarine__actor_attributes.md#standardese-ultramarine__local_actor) - Actor attribute base class that specify that the Derived actor should be treated as a local actor
    
      - [`message_buffer`](doc_ultramarine__utility.md#standardese-ultramarine__message_buffer-Future-) - A dynamic message buffer storing a set number of futures running concurrently
    
      - [`non_reentrant_actor`](doc_ultramarine__actor_attributes.md#standardese-ultramarine__non_reentrant_actor) - Actor attribute base class that specify that the Derived actor should be protected against reentrancy
    
      - [`poly_actor_ref`](doc_ultramarine__actor_ref.md#standardese-ultramarine__poly_actor_ref) - A movable and copyable type-erased reference to a virtual actor.
    
      - [`round_robin_local_placement_strategy`](doc_ultramarine__directory.md#standardese-ultramarine__round_robin_local_placement_strategy) - A round-robin placement strategy that shards actors based on the modulo of their [`ultramarine::actor::KeyType`](doc_ultramarine__actor.md#standardese-ultramarine__actor__KeyType)
    
      - [`with_buffer`](doc_ultramarine__utility.md#standardese-ultramarine__with_buffer-Func--std__size_t-Func---) - Create a [`ultramarine::message_buffer`](doc_ultramarine__utility.md#standardese-ultramarine__message_buffer-Future-) to use in a specified function scope
