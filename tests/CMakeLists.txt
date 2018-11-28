
#file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(test-message_passing message_passing.cpp unit-testing.hpp)
target_link_libraries(test-message_passing PRIVATE ultramarine Catch2::Catch2)