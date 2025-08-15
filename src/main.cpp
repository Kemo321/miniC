#include "miniC/Lexer.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: cminusminus <input.cmm>\n";
        return 1;
    }
    std::cout << "Compiling: " << argv[1] << "\n";
    return 0;
}