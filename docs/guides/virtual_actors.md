---
layout: default
parent: Getting Started
---

# Taking advantage of virtual actors

In the [last section](simple_app.md), we developed a *naive* implementation of a Fibonacci sequence actor. Naive because, generally, a proper *recursive* implementation should use memoization to cache intermediate results, and speed-up the computation.

However, at first sight, a memoization approach will be difficult to implement under the rules of the actor model where sharing memory (a cache, for example) is forbidden. We could create a central cache actor, but because actors are single-threaded by design, it would introduce a central bottleneck.

Fortunately, a virtual actor model provides *horizontal* actors. Therefore, in Ultramarine, two calls to `ultramarine::get<fibonacci_actor>(index_number)` will always point to the same actor. We can take advantage of this by having each actor remember it's value.

## Introducing caching

First, we need to introduce a state into our actor:

```cpp
class fibonacci_actor : public ultramarine::actor<fibonacci_actor> {
    std::optional<int> result;
public:
    ULTRAMARINE_DEFINE_ACTOR(fibonacci_actor, (fib));
    seastar::future<int> fib(); // compute Fibonacci number for index `this->key`
};
```

The variable `result` can now store the Fibonacci number specific to this actor. We modify our message handler to store its result and reuse it upon further messages:

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

## Performance caparison

Executing the above code a couple of time and averaging the results, we get:

| Index number | No caching | Cached  | Speedup |
|--------------|------------|---------|---------|
| 24           | 9661 us    | 952 us  | ~10x    |
| 25           | 15327 us   | 1160 us | ~13x    |
| 26           | 24957 us   | 1427 us | ~17x    |
| 27           | 41202 us   | 1893 us | ~22x    |

