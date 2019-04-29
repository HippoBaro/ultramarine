---
title: Getting Started
nav_order: 2
---

# Getting Started

In an actor application, an actor is the fundamental unit of computation. It’s the *thing* that receives a *message* and does some computation based on it. The idea is very similar to what we have in object-oriented languages: an object receives a method call and does something depending on which method was called.

The main differences are that actors:
 - Are entirely isolated from each other, sharing no resources (memory or otherwise). An actor maintains a private state that may never be mutated directly by another entity.
 - Own their execution context. In contrast with a method call performed on an object, the calling thread *will not* enter the actor; instead, the actor shall *eventually* perform the requested action. This allows actor application to run lock-free.

In a nutshell, *Actors* are named because they ***act*** by themselves, in contrast with *objects* that are just resources ***acted upon*** by *threads*. Akka, a JVM-based actor framework has an excellent primer on the actor model, it is [available here](https://doc.akka.io/docs/akka/current/guide/actors-motivation.html).

## Difference with other implementations

Ultramarine differs from other actor system implementations in a few key points:
 - **Messages are expressed as [`futures`](http://docs.seastar.io/master/group__future-module.html).** There are no one-way messages in Ultramarine, and all message handler can return results.
 - **Virtual actors are completely horizontal.** Error propagation method choice is left to the developer (`exception`, `optional`, `expected`, etc.)
 - **Messages are not objects.** Unlike in [Erlang](https://www.erlang.org/course/concurrent-programming#messages) or [Akka](https://doc.akka.io/docs/akka/new-docs-quickstart-snapshot/define-actors.html), messages in Ultramarine are simple function calls with optional arguments.
 - **Actors do not have individual mailboxes.** Ultramarine does not assign mailboxes to actors for various reasons. Rather, each execution unit (reactor thread) have a single *task queue* in which messages are scheduled.
 - **Actors cooperate, rather than being preempted.** Blocking in actor code is, therefore, a big no-no. Fortunately, Seastar provides [lots of nice constructs](http://docs.seastar.io/master/group__future-util.html) to help with this.

## Developing a simple actor application

As an example, let's create a simple program computing different Fibonacci index numbers.

In this example, we will use the naive divide-and-conquer approach of spawning actors, each responsible for computing a single index number.
They then spawn other actors, as necessary. We can visualize the computation as such:

![](https://upload.wikimedia.org/wikipedia/commons/a/a3/Call_Tree_for_Fibonacci_Number_F6.svg)

Here is an empty `fibinacci_actor` actor definition:

```cpp
class fibonacci_actor : public ultramarine::actor<fibonacci_actor> {
public:
    ULTRAMARINE_DEFINE_ACTOR(fibonacci_actor, (fib));
    seastar::future<int> fib(); // compute Fibonacci number for index `this->key`
};
```

Notice that in Ultramarine, actors:
 - are simple `class/struct` that inherit from [`ultramarine::actor`](/api/doc_ultramarine__actor.md#standardese-ultramarine__actor).
 - need some implementation details hidden behind the macro [`ULTRAMARINE_DEFINE_ACTOR`](/api/doc_ultramarine__macro.md#standardese-ULTRAMARINE_DEFINE_ACTOR)
 - have a unique `key`. By default the type of actor's key is a long unsigned int (see [`KeyType` in ultramarine::actor](/api/doc_ultramarine__actor.md#standardese-ultramarine__actor))

Now, we can call our actor from anywhere in our Seastar code. To call an actor, we first need to create a reference to it:

```cpp
auto ref = ultramarine::get<fibonacci_actor>(24);
return ref.tell(fibonacci_actor::message::fib()).then([] (int value) {
    seastar::print("Result: %d\n", value);
});
```

An [`actor reference`](/api/doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) is a trivial object that you can use to refer to an actor. They are forgeable, copiable and movable. Because Ultramarine is a [Virtual Actor](http://research.microsoft.com/apps/pubs/default.aspx?id=210931) framework, you do not need to *create* any actor. They are created as needed.

Now that the interface and calling site for our actor is set-up, we can implement `fibonacci_actor::fib`:

```cpp
seastar::future<int> fibonacci_actor::fib() {
    if (key <= 2) {
        return seastar::make_ready_future<int>(1);
    } else {
        auto f1 = ultramarine::get<fibonacci_actor>(key - 1).tell(fibonacci_actor::message::fib());
        auto f2 = ultramarine::get<fibonacci_actor>(key - 2).tell(fibonacci_actor::message::fib());
        return seastar::when_all_succeed(std::move(f1), std::move(f2)).then([] (auto r1, auto r2) {
            return r1 + r2;
        });
    }
}
```

That's it! To compute our Fibonacci number, we divide the problem in two and delegate it to two other actors. Once they are done, we combine the result and return. Better yet, this code is natively multi-threaded, because all actors are evenly spread throughout all available hardware.

## Taking advantage of virtual actors

In the last section, we developed a *naive* implementation of a Fibonacci sequence actor. Naive because, generally, a proper *recursive* implementation should use memoization to cache intermediate results, and speed-up the computation.

However, at first sight, a memoization approach will be difficult under the rules of the actor model where sharing memory (a cache, for example) is forbidden. We could create a central actor and have it act as a cache actor, but it would introduce a central bottleneck.

Fortunately, a virtual actor model provides *horizontal* actors. Therefore, in Ultramarine, two calls to `ultramarine::get<fibonacci_actor>(index_number)` will always point to the same actor. We can take advantage of this by having each actor remember it's value.

First, we need to introduce a state into our actor:

```cpp
class fibonacci_actor : public ultramarine::actor<fibonacci_actor> {
    std::optional<int> result;
public:
    ULTRAMARINE_DEFINE_ACTOR(fibonacci_actor, (fib));
    seastar::future<int> fib(); // compute Fibonacci number for index `this->key`
};
```

The variable `result` can now store the Fibonacci number specific to this actor. We modify our message handler to store its result and reuse it upon further message:

```cpp
seastar::future<int> fibonacci_actor::fib() {
    if (key <= 2) {
        return seastar::make_ready_future<int>(1);
    } else if (result) {
        return seastar::make_ready_future<int>(*result);
    } else {
        auto f1 = ultramarine::get<fibonacci_actor>(key - 1).tell(fibonacci_actor::message::fib());
        auto f2 = ultramarine::get<fibonacci_actor>(key - 2).tell(fibonacci_actor::message::fib());
        return seastar::when_all_succeed(std::move(f1), std::move(f2)).then([this] (auto r1, auto r2) {
            return result = r1 + r2;
        });
    }
}
```

Executing the above code a couple of time and averaging the results, we get:

| Index number | No caching | Cached  | Speedup |
|--------------|------------|---------|---------|
| 24           | 9661 us    | 952 us  | ~10x    |
| 25           | 15327 us   | 1160 us | ~13x    |
| 26           | 24957 us   | 1427 us | ~17x    |
| 27           | 41202 us   | 1893 us | ~22x    |

