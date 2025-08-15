# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Define test executable
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(minic_tests ${TEST_SOURCES})

# Link against Google Test and compiler sources
target_link_libraries(minic_tests PRIVATE gtest gtest_main)

# Include directories for compiler headers
target_include_directories(minic_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Add test
add_test(NAME miniCTests COMMAND minic_tests)