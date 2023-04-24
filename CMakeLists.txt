cmake_minimum_required(VERSION 3.23)
project(btree_clion)

set(CMAKE_CXX_STANDARD 17)

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
        btree_test
        tests/test_summed_indexed.cpp
        tests/test_iterators.cpp
        tests/test_min.cpp
        tests/test_ordered.cpp
        tests/test_uninitialized_array.cpp
        tests/test_reference_wrapper.cpp
        tests/test_iterator_insert_erase.cpp
        tests/test_deque.cpp
        tests/test_persistent_random_modifications.cpp
        tests/test_sum_lower_bound.cpp tests/test_inverted_index.cpp)

target_include_directories(btree_test PRIVATE include)
target_include_directories(btree_test PRIVATE examples)

target_link_libraries(btree_test GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(btree_test)

target_compile_options(btree_test PRIVATE -Og)
target_compile_options(btree_test PRIVATE -Wall)
target_compile_options(btree_test PRIVATE -Wextra)
target_compile_options(btree_test PRIVATE -Wconversion)
target_compile_options(btree_test PRIVATE -Wsign-conversion)
target_compile_options(btree_test PRIVATE -Wfloat-conversion)
target_compile_options(btree_test PRIVATE -Werror)
target_compile_options(btree_test PRIVATE -fsanitize=undefined)
target_link_options(btree_test PRIVATE -fsanitize=undefined)
target_compile_definitions(btree_test PRIVATE BPPTREE_TEST_COUNT_ALLOCATIONS)

#target_compile_options(btree_clion PRIVATE -ftime-trace)
#target_link_options(btree_clion PRIVATE -ftime-trace)

if(ENABLE_TEST_COVERAGE)
    target_compile_options(btree_test PRIVATE -g -fprofile-arcs -ftest-coverage)
    target_link_options(btree_test PRIVATE -fprofile-arcs -ftest-coverage)
endif()
