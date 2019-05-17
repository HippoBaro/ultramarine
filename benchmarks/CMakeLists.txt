function(add_ultramarine_benchmark)
    set (options CLUSTERED)
    set (one_value_args NAME)
    set (multi_value_args SOURCES ARGS)
    cmake_parse_arguments (args "${options}" "${one_value_args}" "${multi_value_args}" "${ARGN}")

    add_executable(${args_NAME} "${args_SOURCES}" benchmark_utility.hpp)
    target_link_libraries(${args_NAME} PRIVATE Ultramarine::actor)
    add_dependencies (ultramarine-benchmarks ${args_NAME})

    if (${args_CLUSTERED})
        add_executable(${args_NAME}_clustered "${args_SOURCES}" benchmark_utility.hpp)
        target_link_libraries(${args_NAME}_clustered PRIVATE Ultramarine::cluster)
        add_dependencies (ultramarine-benchmarks ${args_NAME}_clustered)
        target_compile_definitions(${args_NAME}_clustered PRIVATE CLUSTERED_BENCHMARK)
    endif()
endfunction()

# Logical target for all benchmarks.
add_custom_target (ultramarine-benchmarks)

add_custom_command(
        TARGET ultramarine-benchmarks POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/run_cluster_bench.py
        ${CMAKE_CURRENT_BINARY_DIR}/run_cluster_bench.py)

add_custom_command(
        TARGET ultramarine-benchmarks POST_BUILD
        COMMAND chmod 751 ${CMAKE_CURRENT_BINARY_DIR}/run_cluster_bench.py)

add_ultramarine_benchmark(NAME message_passing SOURCES message_passing.cpp CLUSTERED)
add_ultramarine_benchmark(NAME skynet SOURCES skynet.cpp CLUSTERED)
add_ultramarine_benchmark(NAME actor_creation SOURCES actor_creation.cpp CLUSTERED)
add_ultramarine_benchmark(NAME fibonacci SOURCES fibonacci.cpp CLUSTERED)
add_ultramarine_benchmark(NAME philosophers SOURCES philosophers.cpp CLUSTERED)
add_ultramarine_benchmark(NAME ping_pong SOURCES ping_pong.cpp CLUSTERED)
add_ultramarine_benchmark(NAME counting SOURCES counting.cpp CLUSTERED)
add_ultramarine_benchmark(NAME fork-join_throughput SOURCES fork-join_throughput.cpp CLUSTERED)
add_ultramarine_benchmark(NAME fork-join_create SOURCES fork-join_create.cpp CLUSTERED)
add_ultramarine_benchmark(NAME thread_ring SOURCES thread_ring.cpp CLUSTERED)
add_ultramarine_benchmark(NAME big SOURCES big.cpp CLUSTERED)
add_ultramarine_benchmark(NAME mailbox_performance SOURCES mailbox_performance.cpp CLUSTERED)
