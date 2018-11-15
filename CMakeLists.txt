cmake_minimum_required (VERSION 3.1)
project (ultramarine CXX)

# Options
option(ULTRAMARINE_BUILD_APPS "Include apps/ as part of the build" ON)

# Dependencies
set(SEASTAR_ENABLE_TESTS OFF CACHE BOOL "Not testing Seastar")
set(SEASTAR_ENABLE_WERROR OFF CACHE BOOL "No error on warning")
set(SEASTAR_USE_STD_OPTIONAL_VARIANT_STRINGVIEW ON CACHE BOOL "Use std over boost when possible")
add_subdirectory (seastar)

# Ultramarine
add_library(ultramarine
        src/actor.cpp src/actor.hpp
        src/silo.cpp src/silo.hpp
        )
target_link_libraries(ultramarine PUBLIC Seastar::seastar)
target_include_directories(ultramarine INTERFACE src)

# Targets
if (${ULTRAMARINE_BUILD_APPS})
    add_subdirectory(apps)
endif()