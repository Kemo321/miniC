#include "minic/CodeGenerator.hpp"
#include "minic/IRGenerator.hpp"
#include "minic/Lexer.hpp"
#include "minic/Parser.hpp"
#include "minic/SemanticAnalyzer.hpp"
#include <fstream>
#include <iostream>
#include <memory>

int compile_file(const std::string& filename, const std::string& source);

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: cminusminus <input.cmm>\n";
        return 1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file)
    {
        std::cerr << "Error: Could not open input file '" << argv[1] << "'.\n";
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
    return compile_file(argv[1], source);
}

int compile_file(const std::string& filename, const std::string& source)
{
    std::cout << "Compiling: " << filename << "\n";

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
        std::cerr << "Error during semantic analysis: " << e.what() << "\n";
        return 1;
    }

    std::unique_ptr<minic::IRProgram> ir_program;
    try
    {
        minic::IRGenerator ir_gen;
        ir_program = ir_gen.generate(*program);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error during IR generation: " << e.what() << "\n";
        return 1;
    }

    try
    {
        minic::CodeGenerator code_gen;
        code_gen.generate(*ir_program, "output.asm");
        std::cout << "Assembly generated to output.asm\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error during code generation: " << e.what() << "\n";
        return 1;
    }

    return 0;
}