#pragma once

#include <string>
#include <fstream>
#include <vector>

enum class TokenType {
    Identifier,
    Keyword,
    Int,
    Dec,
    Char,
    String,
    Symbol,
    EndOfFile,
    Unknown
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;
};

extern const std::vector<std::string> keywords;

class Lexer {
public:
    Lexer(std::ifstream& input);

    Token nextToken();
    void reset();

private:
    std::ifstream& file;
    int line;
    int col;

    void skipIgnored();
    Token readIdentifier();
    Token readNumber();
    Token readSymbol();
    Token readString();
    Token readChar();
};
