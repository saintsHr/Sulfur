#include <iostream>

#include "error.hpp"

void printErr(const error& err) {
    std::cout << "\n\033[91mCompiler ran into an error:\033[0m\n";

    std::cout << "[Code]:    " << err.code << "\n";
    std::cout << "[Source]:  " << err.source << "\n";
    std::cout << "[Message]: " << err.msg << "\n";

    if (err.hint)   std::cout << "[Hint]:    " << *err.hint << "\n";
    if (err.file)   std::cout << "[File]:    " << *err.file << "\n";
    if (err.line)   std::cout << "[Line]:    " << *err.line << "\n";
    if (err.line)   std::cout << "[Coll]:    " << *err.col << "\n";

    std::cout << "\n";
}