#include <optional>
#include <string.h>
#include <iostream>

struct error{
    int code;
    std::string msg;
    std::optional<std::string> hint;
    std::optional<std::string> file;
    std::optional<int> line;
};

void printErr(const error& err);