#include "minic/Lexer.hpp"
#include "minic/Parser.hpp"
#include "minic/SemanticAnalyzer.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: minic <input.mc>\n";
        return 1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file)
    {
        std::cerr << "Error: Could not open input file.\n";
        return 1;
    }

    std::cout << "Compiling: " << argv[1] << "\n";

    std::string source((std::istreambuf_iterator<char>(input_file)),
        std::istreambuf_iterator<char>());

    std::vector<minic::Token> tokens;
    try
    {
        minic::Lexer lexer(source);
        tokens = lexer.Lex();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error while lexing: " << e.what() << "\n";
        return 1;
    }

    std::unique_ptr<minic::Program> program;
    try
    {
        minic::Parser parser(tokens);
        program = parser.parse();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error while parsing: " << e.what() << "\n";
        return 1;
    }

    try
    {
        minic::SemanticAnalyzer analyzer;
        analyzer.visit(*program);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error while analyzing semantics: " << e.what() << "\n";
        return 1;
    }

    return 0;
}