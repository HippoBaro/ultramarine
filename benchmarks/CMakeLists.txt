add_executable(message_passing message_passing.cpp benchmark_utility.hpp)
target_link_libraries(message_passing PRIVATE Ultramarine::actor)

add_executable(skynet skynet.cpp benchmark_utility.hpp)
target_link_libraries(skynet PRIVATE Ultramarine::actor)

add_executable(actor_creation actor_creation.cpp benchmark_utility.hpp)
target_link_libraries(actor_creation PRIVATE Ultramarine::actor)

add_executable(fibonacci fibonacci.cpp benchmark_utility.hpp)
target_link_libraries(fibonacci PRIVATE Ultramarine::actor)

add_executable(philosophers philosophers.cpp benchmark_utility.hpp)
target_link_libraries(philosophers PRIVATE Ultramarine::actor)

add_executable(ping_pong ping_pong.cpp benchmark_utility.hpp)
target_link_libraries(ping_pong PRIVATE Ultramarine::actor)

add_executable(counting counting.cpp benchmark_utility.hpp)
target_link_libraries(counting PRIVATE Ultramarine::actor)

add_executable(fork-join_throughput fork-join_throughput.cpp benchmark_utility.hpp)
target_link_libraries(fork-join_throughput PRIVATE Ultramarine::actor)

add_executable(fork-join_create fork-join_create.cpp benchmark_utility.hpp)
target_link_libraries(fork-join_create PRIVATE Ultramarine::actor)