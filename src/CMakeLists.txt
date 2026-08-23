# src/CMakeLists.txt

set(MINIC_SOURCES
    main.cpp
    Lexer.cpp
    Parser.cpp
    SemanticAnalyzer.cpp
    IRGenerator.cpp
    CodeGenerator.cpp
)

add_executable(minic ${MINIC_SOURCES})

set_target_properties(minic PROPERTIES CMAKE_CXX_CLANG_TIDY "clang-tidy")

target_include_directories(minic PRIVATE ${CMAKE_SOURCE_DIR}/include)

install(TARGETS minic DESTINATION bin)
