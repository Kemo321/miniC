cmake_minimum_required(VERSION 3.18)
project(miniC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # For IDEs/clangd

# To turn on production mode, set the PRODUCTION option to ON
# Compile it with : cmake -DPRODUCTION=ON ..
option(PRODUCTION "Build in production mode" OFF)
if(MSVC)
    add_compile_options(/W4)
elseif(PRODUCTION)
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
else()
    # Developer mode: debug symbols + AddressSanitizer + UndefinedBehaviorSanitizer
    add_compile_options(
        -g
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    add_link_options(-fsanitize=address,undefined)
endif()


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

# ---------------------------------------------------------------------------
# Doxygen HTML documentation (headers in include/minic/)
# Build with: cmake --build . --target docs
# ---------------------------------------------------------------------------
option(BUILD_DOCS "Generate Doxygen HTML documentation" ON)
if(BUILD_DOCS)
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_PROJECT_NAME "miniC")
        set(DOXYGEN_PROJECT_BRIEF "A lightweight compiler for a simplified C-like language")
        set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
        set(DOXYGEN_RECURSIVE YES)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX NO)
        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_DISABLE_INDEX NO)
        set(DOXYGEN_FULL_SIDEBAR NO)
        set(DOXYGEN_HTML_COLORSTYLE "AUTO")
        set(DOXYGEN_HTML_DYNAMIC_SECTIONS YES)
        set(DOXYGEN_EXTRACT_ALL NO)
        set(DOXYGEN_EXTRACT_PRIVATE YES)
        set(DOXYGEN_EXTRACT_STATIC YES)
        set(DOXYGEN_QUIET YES)
        set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
        set(DOXYGEN_FILE_PATTERNS "*.hpp" "*.h")
        set(DOXYGEN_EXCLUDE_PATTERNS "*/build/*")
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
        set(DOXYGEN_SOURCE_BROWSER YES)
        set(DOXYGEN_INLINE_SOURCES NO)
        set(DOXYGEN_STRIP_FROM_PATH "${CMAKE_SOURCE_DIR}")
        set(DOXYGEN_STRIP_FROM_INC_PATH "${CMAKE_SOURCE_DIR}/include")
        set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
        set(DOXYGEN_BUILTIN_STL_SUPPORT YES)

        doxygen_add_docs(
            docs
            "${CMAKE_SOURCE_DIR}/include/minic"
            "${CMAKE_SOURCE_DIR}/README.md"
            COMMENT "Generating miniC API documentation with Doxygen"
        )
    else()
        message(STATUS "Doxygen not found — 'docs' target will be unavailable")
    endif()
endif()

# Optional: Install
install(TARGETS minic DESTINATION bin)
