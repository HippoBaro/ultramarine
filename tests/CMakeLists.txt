function(add_ultramarine_test)
    set(options CUSTOM SUITE)
    set(one_value_args NAME)
    set(multi_value_args SOURCES ARGS)
    cmake_parse_arguments(args "${options}" "${one_value_args}" "${multi_value_args}" "${ARGN}")

    add_seastar_test(${ARGN})
    target_link_libraries(${args_NAME} PRIVATE ultramarine)
    target_link_libraries(${args_NAME}_g PRIVATE ultramarine)
endfunction()

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_ultramarine_test(NAME test-message_passing
        SUITE
        SOURCES message_passing.cpp)