cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
include(Cooking OPTIONAL)

project(ultramarine CXX)
set(CMAKE_CXX_STANDARD 17)

# Options
option(ULTRAMARINE_BUILD_APPS "Include apps/ as part of the build" ON)
option(ULTRAMARINE_BUILD_UNITTESTS "Include tests/ as part of the build" ON)
option(ULTRAMARINE_BUILD_BENCHMARKS "Include benchmarks/ as part of the build" ON)
option(ULTRAMARINE_BUILD_DOC "Generate API definitions in docs/" OFF)

# Dependencies
find_package(Seastar 1.0 REQUIRED)

# Ultramarine
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_library(ultramarine-actor INTERFACE)
target_link_libraries(ultramarine-actor INTERFACE Seastar::seastar)
target_include_directories(ultramarine-actor
        INTERFACE include
        )
add_library(Ultramarine::actor ALIAS ultramarine-actor)

# Apps
if (${ULTRAMARINE_BUILD_APPS})
    add_subdirectory(apps)
endif ()

# Unit tests
if (${ULTRAMARINE_BUILD_UNITTESTS})
    enable_testing()
    add_subdirectory(tests EXCLUDE_FROM_ALL)
endif ()

# Benchmarks
if (${ULTRAMARINE_BUILD_BENCHMARKS})
    add_subdirectory(benchmarks)
endif ()

# Documentation
if (${ULTRAMARINE_BUILD_DOC})
    if (NOT DEFINED STANDARDESE_DIR)
        message(FATAL_ERROR "STANDARDESE_DIR is not defined")
    endif ()
    set(STANDARDESE_TOOL ${STANDARDESE_DIR}/build/tool/standardese)
    include(${STANDARDESE_DIR}/standardese-config.cmake)

    get_target_property(includes Seastar::seastar INTERFACE_INCLUDE_DIRECTORIES)
    standardese_generate(ultramarine-actor CONFIG docs/standardese.config INCLUDE_DIRECTORY ${includes} include INPUT include)

    add_custom_command(TARGET standardese_ultramarine-actor POST_BUILD
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/docs/gen.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/standardese_ultramarine-actor
            COMMENT "Editing API doc for web hosting..."
            )
    add_custom_command(TARGET standardese_ultramarine-actor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_BINARY_DIR}/standardese_ultramarine-actor ${CMAKE_CURRENT_SOURCE_DIR}/docs/api)
endif ()