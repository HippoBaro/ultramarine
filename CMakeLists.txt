#
# MIT License
#
# Copyright (c) 2018 Hippolyte Barraud
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake
        ${CMAKE_CURRENT_BINARY_DIR})
include(Cooking OPTIONAL)

project(ultramarine CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(ULTRAMARINE_BUILD_APPS "Include apps/ as part of the build" ON)
option(ULTRAMARINE_BUILD_UNITTESTS "Include tests/ as part of the build" ON)
option(ULTRAMARINE_BUILD_BENCHMARKS "Include benchmarks/ as part of the build" ON)
option(ULTRAMARINE_BUILD_DOC "Generate API definitions in docs/" OFF)

# Dependencies
find_package(Seastar 1.0 REQUIRED)
find_package(HashRing REQUIRED)

# Ultramarine actors
add_library(ultramarine-actor INTERFACE)
target_link_libraries(ultramarine-actor INTERFACE Seastar::seastar)
target_include_directories(ultramarine-actor
        INTERFACE include
        )
add_library(Ultramarine::actor ALIAS ultramarine-actor)

# Ultramarine cluster
add_library(ultramarine-cluster STATIC
        src/server.cpp
        src/membership.cpp
        src/node.cpp
        src/handshake.cpp)
target_include_directories(ultramarine-cluster PRIVATE include)
target_link_libraries(ultramarine-cluster PUBLIC Seastar::seastar INTERFACE Ultramarine::actor PRIVATE hashring::hashring Concepts::concepts)
target_compile_definitions(ultramarine-cluster INTERFACE ULTRAMARINE_REMOTE)
add_library(Ultramarine::cluster ALIAS ultramarine-cluster)

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