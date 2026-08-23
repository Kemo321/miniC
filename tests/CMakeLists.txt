# tests/CMakeLists.txt

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(MINIC_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/Lexer.cpp
    ${CMAKE_SOURCE_DIR}/src/Parser.cpp
    ${CMAKE_SOURCE_DIR}/src/SemanticAnalyzer.cpp
    ${CMAKE_SOURCE_DIR}/src/IRGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/CodeGenerator.cpp
)

set(MINIC_TEST_SOURCES
    main.cpp
    TestAST.cpp
    TestLexer.cpp
    TestParser.cpp
    TestSemanticAnalyzer.cpp
    TestIRGenerator.cpp
)

add_executable(minic_tests
    ${MINIC_TEST_SOURCES}
    ${MINIC_LIB_SOURCES}
)

target_link_libraries(minic_tests PRIVATE gtest gtest_main)

target_include_directories(minic_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME miniCTests COMMAND minic_tests)
