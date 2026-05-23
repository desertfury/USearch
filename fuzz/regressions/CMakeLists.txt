# Builds every `test_*.c` file in this directory as a standalone executable,
# links it against an inline build of `c/lib.cpp`, and registers it with
# CTest. Each test reproduces a specific fuzzer-found bug — failing means
# the regression has come back.
file(GLOB USEARCH_REGRESSION_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.c")

if (NOT USEARCH_REGRESSION_TESTS)
    return ()
endif ()

include(CTest)
enable_testing()

foreach (test_src ${USEARCH_REGRESSION_TESTS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src} ${CMAKE_SOURCE_DIR}/c/lib.cpp)
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/c)
    setup_target(${test_name})
    # Keep the per-test timeout tight so the rename / iterator-loop
    # regressions surface quickly if they ever come back.
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30)
endforeach ()
