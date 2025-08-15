cmake_minimum_required(VERSION 3.18)
project(miniC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # For IDEs/clangd

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

# Include directories for headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add compiler subdirectory
add_subdirectory(src)

# Add tests subdirectory (optional)
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Optional: Install
install(TARGETS minic DESTINATION bin)