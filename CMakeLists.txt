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
add_library(ultramarine
        src/actor.cpp src/actor.hpp
        src/silo.cpp src/silo.hpp
        src/actor_ref.cpp src/actor_ref.hpp
        src/macro.hpp
        )
target_link_libraries(ultramarine PUBLIC Seastar::seastar)
target_include_directories(ultramarine INTERFACE src)

# Apps
if (${ULTRAMARINE_BUILD_APPS})
    add_subdirectory(apps)
endif()

# Unit tests
if (${ULTRAMARINE_BUILD_UNITTESTS})
    add_subdirectory(tests)
endif()

# Benchmarks
if (${ULTRAMARINE_BUILD_BENCHMARKS})
    add_subdirectory(benchmarks)
endif()