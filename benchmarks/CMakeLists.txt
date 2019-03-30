
#file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(message_passing message_passing.cpp benchmark_utility.hpp)
target_link_libraries(message_passing PRIVATE Ultramarine::actor)

add_executable(skynet skynet.cpp benchmark_utility.hpp)
target_link_libraries(skynet PRIVATE Ultramarine::actor)

add_executable(actor_creation actor_creation.cpp benchmark_utility.hpp)
target_link_libraries(actor_creation PRIVATE Ultramarine::actor)