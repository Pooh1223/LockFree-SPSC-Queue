cmake_minimum_required(VERSION 3.10)

project(SPSC_RingBuffer)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -g")
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
else()
    add_compile_options(-g -fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

include_directories(include)

find_package(Threads REQUIRED)

add_executable(spsc_test src/main.cpp)
add_executable(spsc_perf src/perf_spsc.cpp)
add_executable(mpmc_perf src/perf_mpmc.cpp)
add_executable(unit_tests tests/correctness_test.cpp)

target_link_libraries(unit_tests GTest::gtest_main)
target_link_libraries(spsc_test PRIVATE Threads::Threads)
target_link_libraries(spsc_perf PRIVATE Threads::Threads)
target_link_libraries(mpmc_perf Threads::Threads)