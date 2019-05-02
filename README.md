<p align="center"><img src="https://hippobaro.github.io/ultramarine/assets/logo.png" alt="Ultramarine logo" width="256px" height="256px"></p>


[![CircleCI branch](https://img.shields.io/circleci/project/github/HippoBaro/ultramarine/master.svg?color=%23120a8f&style=for-the-badge)](https://circleci.com/gh/HippoBaro/ultramarine)
[![Licence](https://img.shields.io/github/license/HippoBaro/ultramarine.svg?color=%23120a8f&style=for-the-badge)](https://github.com/HippoBaro/ultramarine/blob/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/HippoBaro/ultramarine.svg?color=%23120a8f&style=for-the-badge)](https://github.com/HippoBaro/ultramarine/releases)

Ultramarine is a lightweight modern actor library built on top of the [Seastar C++ framework](https://github.com/scylladb/seastar). It helps writing distributed applications using virtual actors. It allows developers to write highly scalable applications while greatly simplifying discovery, state management, actor lifetime and more.

The [Virtual Actor Model](http://research.microsoft.com/apps/pubs/default.aspx?id=210931) is a variation of the [Actor Model](https://en.wikipedia.org/wiki/Actor_model) where "virtual" actors (transcendental entities combining state and execution context) share and process information by exchanging messages. 

It is heavily inspired by the [Microsoft Orleans](https://dotnet.github.io/orleans/Documentation/index.html) project.

> Ultramarine is a work-in-progress.

# Installation

Ultramarine is built upon [Seastar](https://github.com/scylladb/seastar) and share the same dependencies. Seastar provides a convenience script to pull all necessary packages (`install-dependencies.sh`).

To pull Seastar and configure Ultramarine:

```
./cooking.sh -t Release
```

To build the examples:

```
ninja -C build
```

# Documentation

Various guides, examples and API reference are [available here](https://hippobaro.github.io/ultramarine/).

# Code Example

First we need to define an [`actor`](https://hippobaro.github.io/ultramarine/api/doc_ultramarine__actor/):

```cpp
class hello_actor : public ultramarine::actor<hello_actor> {
public:
    using KeyType = std::string;

    seastar::future<> say_hello() const {
        seastar::print("Hello, %s.\n", key);
        return seastar::make_ready_future();
    }

    ULTRAMARINE_DEFINE_ACTOR(hello_actor, (say_hello));
};
```

And then call the actor activation from anywhere in your seastar code using an [`actor_ref`](https://hippobaro.github.io/ultramarine/api/doc_ultramarine__actor_ref#standardese-ultramarine__actor_ref-Actor-):

```cpp
auto ref = ultramarine::get<hello_actor>("Ultramarine");
auto future = ref->say_hello();
// wait or attach a continuation to the returned future.
```

# Performance

Ultramarine is build on Seastar and benefits from a lock-free, [shared-nothing design](http://seastar.io/shared-nothing/). Compared to typical actor model implementations, it doesn't use any locking or complex cache-unfriendly concurrent data-structures internally.

Specifically, this gives Ultramarine an advantage on many-to-many communication patterns, because there is not contention on mailboxes. Also, because Ultramarine doesn't have per-actor mailboxes and actor' messages aren't processed in batches, it has better latency characteristics.

As an example see how it compares against other popular actor libraries on the [Big actor benchmark](http://release.softlab.ntua.gr/bencherl/files/erlang01-aronis.pdf):

Mean Execution Time        | Standard deviation
---------------------------|--------------------
![](https://hippobaro.github.io/ultramarine/assets/big_met.png)    | ![](https://hippobaro.github.io/ultramarine/assets/big_std.png)

More information and benchmarks are [available here](https://hippobaro.github.io/ultramarine/benchmarks).

# Going forward

Ultramarine is a small project and currently lacks:
- [ ] **Clustering.** An actor system on a local machine is nice, but the concept shines when actors are allowed to migrate across a set of clustered machines freely.
- [ ] **Persistence.** Actors could write their states in storage or non-volatile memory to provide failure-safety.
- [ ] **Steaming.** Message-passing doesn't always map well to domain-specific problems. A streaming API would be nice to have.
- [ ] **Actor as heterogeneous compute abstraction.** Actors are useful to abstract over hardware because the abstraction's requirements are very loose. [CAF](https://actor-framework.org/) has demonstrated that actors work well to abstract a GPU.

# License

This project is licensed under the [MIT license](https://github.com/HippoBaro/ultramarine/blob/master/LICENSE).


