---
layout: default
parent: Getting Started
---

# Developing a simple actor application

As an example, let's create a simple program computing different [Fibonacci numbers](https://en.wikipedia.org/wiki/Fibonacci_number).

In this example, we will use the naive divide-and-conquer approach of spawning actors, each responsible for computing a single index number.
They then spawn other actors, as necessary. We can visualize the computation as such:

![](https://upload.wikimedia.org/wikipedia/commons/a/a3/Call_Tree_for_Fibonacci_Number_F6.svg)

## Defining an actor

Here is an empty `fibonacci_actor` actor definition:

```cpp
class fibonacci_actor : public ultramarine::actor<fibonacci_actor> {
public:
    ULTRAMARINE_DEFINE_ACTOR(fibonacci_actor, (fib));
    seastar::future<int> fib(); // compute Fibonacci number for index `this->key`
};
```

Notice that in Ultramarine, actors:
 - are simple `class/struct` that inherit from [`ultramarine::actor`](../api/doc_ultramarine__actor.md#standardese-ultramarine__actor).
 - need some implementation details hidden behind the macro [`ULTRAMARINE_DEFINE_ACTOR`](../api/doc_ultramarine__macro.md#standardese-ULTRAMARINE_DEFINE_ACTOR)
 - have a unique `key`. By default the type of actor's key is a long unsigned int (see [`KeyType` in ultramarine::actor](../api/doc_ultramarine__actor.md#standardese-ultramarine__actor))

## Using actors

Now, we can call our actor from anywhere in our Seastar code. To call an actor, we first need to create a reference to it:

```cpp
auto ref = ultramarine::get<fibonacci_actor>(24);
return ref.tell(fibonacci_actor::message::fib()).then([] (int value) {
    seastar::print("Result: %d\n", value);
});
```

An [`actor reference`](../api/doc_ultramarine__actor_ref.md#standardese-ultramarine__actor_ref-Actor-) is a trivial object that you can use to refer to an actor. They are forgeable, copiable and movable. Because Ultramarine is a [Virtual Actor](http://research.microsoft.com/apps/pubs/default.aspx?id=210931) framework, you do not need to *create* any actor. They are created as needed.

## Message handler implementation

Now that the interface and calling site for our actor are set-up, we can implement `fibonacci_actor::fib`:

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

That's it! To compute our Fibonacci number, we divide the problem in two and delegate to two other actors. Once they are done, we combine the result and return. Better yet, this code is natively multi-threaded, because all actors are evenly spread throughout all available hardware.
