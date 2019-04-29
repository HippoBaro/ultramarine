---
title: Getting Started
nav_order: 2
layout: default
has_children: true
permalink: /getting_started
nav_no_fold : true
---

# Actor system

In an actor application, an actor is the fundamental unit of computation. It’s the *thing* that receives a *message* and does some computation based on it. The idea is very similar to what we have in object-oriented languages: an object receives a method call and does something depending on which method was called.

The main differences are that actors:
 - Are entirely isolated from each other, sharing no resources (memory or otherwise). An actor maintains a private state that may never be mutated directly by another entity.
 - Own their execution context. In contrast with a method call performed on an object, the calling thread *will not* enter the actor; instead, the actor shall *eventually* perform the requested action. This allows actor application to run lock-free.

In a nutshell, *Actors* are named because they ***act*** by themselves, in contrast with *objects* that are just resources ***acted upon*** by *threads*. Akka, a JVM-based actor framework has an excellent primer on the actor model, it is [available here](https://doc.akka.io/docs/akka/current/guide/actors-motivation.html).

# Difference with other implementations

Ultramarine differs from other actor system implementations in a few key points:
 - **Messages are expressed as [`futures`](http://docs.seastar.io/master/group__future-module.html).** There are no one-way messages in Ultramarine, and all message handler can return results.
 - **Virtual actors are completely horizontal.** Error propagation method choice is left to the developer (`exception`, `optional`, `expected`, etc.)
 - **Messages are not objects.** Unlike in [Erlang](https://www.erlang.org/course/concurrent-programming#messages) or [Akka](https://doc.akka.io/docs/akka/new-docs-quickstart-snapshot/define-actors.html), messages in Ultramarine are simple function calls with optional arguments.
 - **Actors do not have individual mailboxes.** Ultramarine does not assign mailboxes to actors for various reasons. Rather, each execution unit (reactor thread) have a single *task queue* in which messages are scheduled.
 - **Actors cooperate, rather than being preempted.** Blocking in actor code is, therefore, a big no-no. Fortunately, Seastar provides [lots of nice constructs](http://docs.seastar.io/master/group__future-util.html) to help with this.
