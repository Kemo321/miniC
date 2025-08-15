# src/CMakeLists.txt
cmake_minimum_required(VERSION 3.18)

# Define the compiler executable
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(minic ${SOURCES})

set_target_properties(minic PROPERTIES CMAKE_CXX_CLANG_TIDY "clang-tidy")

# Ensure include directory is available (redundant but explicit)
target_include_directories(minic PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Install the executable
install(TARGETS minic DESTINATION bin)