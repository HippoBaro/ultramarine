cmake_minimum_required (VERSION 3.1)
project (ultramarine CXX)

add_subdirectory (seastar)

add_executable(main main.cpp)
target_link_libraries(main Seastar::seastar)