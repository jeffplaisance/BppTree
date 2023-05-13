set(CMAKE_CXX_STANDARD 17)

add_executable(benchmarks benchmark.cpp)

target_include_directories(benchmarks PRIVATE ../include)
target_include_directories(benchmarks PRIVATE ../tests)
target_include_directories(benchmarks PRIVATE immer)
target_include_directories(benchmarks PRIVATE abseil-cpp)
target_include_directories(benchmarks PRIVATE tlx)
target_compile_options(benchmarks PRIVATE -fno-exceptions)
