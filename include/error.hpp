#pragma once

#include <optional>
#include <string.h>
#include <iostream>

struct error{
    int code;
    std::string source;
    std::string msg;
    std::optional<std::string> hint;
    std::optional<std::string> file;
    std::optional<int> line;
    std::optional<int> col;
};

void printErr(const error& err);