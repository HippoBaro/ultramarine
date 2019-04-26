---
layout: default
parent: API index
---

# ultramarine/utility.hpp

``` cpp
namespace ultramarine
{
    template <typename Future>
    struct message_buffer;

    template <typename Func>
    auto with_buffer(std::size_t capacity, Func&& func);
}
```

### Struct `ultramarine::message_buffer`

``` cpp
template <typename Future>
struct message_buffer
{
    boost::circular_buffer<Future> futs;

    explicit message_buffer(std::size_t capacity);

    message_buffer(const message_buffer<Future>&) = delete;

    message_buffer(message_buffer<Future>&&) noexcept = default;

    auto operator()(Future&& fut);

    auto flush();
};
```

A dynamic message buffer storing a set number of futures running concurrently

### Template parameter `ultramarine::message_buffer::Future`

``` cpp
typename Future
```

The future type the message buffer will store

*Requires:* Type `Future` shall be a `seastar::future`

-----

### Function `ultramarine::message_buffer::operator()`

``` cpp
auto operator()(Future&& fut);
```

Push a future in the message buffer

### Parameter `ultramarine::message_buffer::fut`

``` cpp
Future&& fut
```

The future to push in the message buffer

*Returns:* An immediately available future, or a unresolved future if the message buffer is full

-----

-----

### Function `ultramarine::message_buffer::flush`

``` cpp
auto flush();
```

Flush the message buffer such that all future it contains are in an available or failed sate

*Returns:* A future

-----

-----

### Function `ultramarine::with_buffer`

``` cpp
template <typename Func>
auto with_buffer(std::size_t capacity, Func&& func);
```

Create a dynamic message buffer to use in a specified function scope

#### Parameters

  - `capacity` - The number of message the buffer should be able to store before awaiting

### Parameter `ultramarine::func`

``` cpp
Func&& func
```

A lambda function using the message buffer

*Returns:* Any value returned by the provided lambda function

-----

-----
