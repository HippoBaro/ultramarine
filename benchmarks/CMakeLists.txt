
#file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(message_passing message_passing.cpp benchmark_utility.hpp)
target_link_libraries(message_passing PRIVATE ultramarine)