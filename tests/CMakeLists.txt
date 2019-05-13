# Adapated from Seastar CMake
add_library (seastar_testing
        ../third-party/seastar/include/seastar/testing/entry_point.hh
        ../third-party/seastar/include/seastar/testing/exchanger.hh
        ../third-party/seastar/include/seastar/testing/seastar_test.hh
        ../third-party/seastar/include/seastar/testing/test_case.hh
        ../third-party/seastar/include/seastar/testing/test_runner.hh
        ../third-party/seastar/include/seastar/testing/thread_test_case.hh
        ../third-party/seastar/src/testing/entry_point.cc
        ../third-party/seastar/src/testing/seastar_test.cc
        ../third-party/seastar/src/testing/test_runner.cc)

add_library (Seastar::seastar_testing ALIAS seastar_testing)

target_compile_definitions (seastar_testing
        PUBLIC BOOST_TEST_DYN_LINK)

target_link_libraries (seastar_testing
        PUBLIC
        Boost::unit_test_framework
        seastar)

function (add_seastar_test)
    set (options CUSTOM SUITE)
    set (one_value_args NAME)
    set (multi_value_args SOURCES ARGS)
    cmake_parse_arguments (args "${options}" "${one_value_args}" "${multi_value_args}" "${ARGN}")

    macro (prepare name)
        add_executable (${name} "${args_SOURCES}")
        target_link_libraries (${name} PUBLIC Seastar::seastar_testing)
        target_compile_definitions (${name}
                PRIVATE SEASTAR_TESTING_MAIN)
    endmacro ()

    prepare (${args_NAME})
    # Strip symbols.
    target_link_libraries (${args_NAME} PUBLIC -s)

    set (name_with_symbols "${args_NAME}_g")
    prepare (${name_with_symbols})
    set_target_properties (${name_with_symbols} PROPERTIES EXCLUDE_FROM_ALL ON)
endfunction ()


function(add_ultramarine_test)
    set(options CUSTOM SUITE)
    set(one_value_args NAME)
    set(multi_value_args SOURCES ARGS)
    cmake_parse_arguments(args "${options}" "${one_value_args}" "${multi_value_args}" "${ARGN}")

    add_seastar_test(${ARGN})
    target_link_libraries(${args_NAME} PRIVATE Ultramarine::actor)
    target_link_libraries(${args_NAME}_g PRIVATE Ultramarine::actor)
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

add_ultramarine_test(NAME test-local_actor
        SOURCES local_actor.cpp)

add_ultramarine_test(NAME test-error_handling
        SOURCES error_handling.cpp)

add_ultramarine_test(NAME test-message_deduplication
        SOURCES message_deduplication.cpp)