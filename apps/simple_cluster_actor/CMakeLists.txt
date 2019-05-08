add_executable(simple_actor_cluster main.cpp simple_actor.cpp)
target_link_libraries(simple_actor_cluster PRIVATE Ultramarine::actor Ultramarine::cluster)