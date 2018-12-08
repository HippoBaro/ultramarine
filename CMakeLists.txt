cmake_minimum_required (VERSION 3.1)
project (ultramarine CXX)
set (CMAKE_CXX_STANDARD 17)

# Options
option(ULTRAMARINE_BUILD_APPS "Include apps/ as part of the build" ON)
option(ULTRAMARINE_BUILD_UNITTESTS "Include tests/ as part of the build" ON)
option(ULTRAMARINE_BUILD_BENCHMARKS "Include benchmarks/ as part of the build" ON)

# Dependencies
add_subdirectory (third-party)

# Ultramarine
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_library(ultramarine-actor INTERFACE)
target_link_libraries(ultramarine-actor INTERFACE Seastar::seastar)
target_include_directories(ultramarine-actor
        INTERFACE include src
        )
add_library (Ultramarine::actor ALIAS ultramarine-actor)

# Apps
if (${ULTRAMARINE_BUILD_APPS})
    add_subdirectory(apps)
endif()

# Unit tests
if (${ULTRAMARINE_BUILD_UNITTESTS})
    enable_testing()
    add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()

# Benchmarks
if (${ULTRAMARINE_BUILD_BENCHMARKS})
    add_subdirectory(benchmarks)
endif()