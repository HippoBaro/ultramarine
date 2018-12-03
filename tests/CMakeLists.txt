function(add_ultramarine_test)
    set(options CUSTOM SUITE)
    set(one_value_args NAME)
    set(multi_value_args SOURCES ARGS)
    cmake_parse_arguments(args "${options}" "${one_value_args}" "${multi_value_args}" "${ARGN}")

    add_seastar_test(${ARGN})
    target_link_libraries(${args_NAME} PRIVATE ultramarine)
    target_link_libraries(${args_NAME}_g PRIVATE ultramarine)
    add_dependencies (ultramarine-tests ${args_NAME})
    add_dependencies (ultramarine-tests-debug ${args_NAME}_g)

    foreach (FILE ${args_SOURCES})
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}" SOURCE_FILE_CONTENTS)
        string(REGEX MATCHALL "SEASTAR_THREAD_TEST_CASE ?\\( *([A-Za-z_0-9]+) *\\)"
                FOUND_TESTS_LOCAL ${SOURCE_FILE_CONTENTS})
        list(APPEND FOUND_TESTS ${FOUND_TESTS_LOCAL})
    endforeach ()

    foreach (HIT ${FOUND_TESTS})
        string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *\\).*" "\\1" TEST_NAME ${HIT})
        message(STATUS "Discovered unit-test ${TEST_NAME}")

        add_test(NAME "${args_NAME}.${TEST_NAME}"
                COMMAND ${args_NAME}
                --run_test=${TEST_NAME} --catch_system_error=yes)
    endforeach ()
endfunction()

# Logical target for all tests.
add_custom_target (ultramarine-tests)
# Logical target for all tests with debugging symbols.
add_custom_target (ultramarine-tests-debug)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_ultramarine_test(NAME test-message_passing
        SOURCES message_passing.cpp)

add_ultramarine_test(NAME test-actor_keys
        SOURCES actor_keys.cpp)