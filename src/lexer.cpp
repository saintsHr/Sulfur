#include "lexer.hpp"
#include <cctype>
#include <vector>

Lexer::Lexer(std::ifstream& input) : file(input), line(1), col(1) {}

void Lexer::skipIgnored() {
    while (true) {
        int c = file.peek();
        if (c == EOF) break;

        if (c == ' ' || c == '\t' || c == '\r'){
            file.get();
            col++;
        }
        else if (c == '\n'){
            file.get();
            line++;
            col = 1;
        }
        else if (c == '/'){
            file.get();
            if (file.peek() == '/'){
                while ((c = file.get()) != '\n' && c != EOF) {}
                line++;
                col = 1;
            }else{
                file.unget();
                break;
            }
        }
        else break;
    }
}

Token Lexer::readIdentifier() {
    std::string value;
    int startCol = col;

    while (std::isalnum(file.peek()) || file.peek() == '_'){
        value.push_back(file.get());
        col++;
    }

    for (int i = 0; i <= sizeof(keywords); i++){
        if (value == keywords[i]){
            return {TokenType::Keyword, value, line, startCol};
        }
    }

    return {TokenType::Identifier, value, line, startCol};
}

Token Lexer::readNumber() {
    std::string value;
    int startCol = col;
    bool isFloat = false;

    while (true) {
        int c = file.peek();
        if (c == EOF) break;

        if (std::isdigit(c)) {
            value.push_back(file.get());
            col++;
        }
        else if ((c == '.' || c == 'e' || c == 'E') && !isFloat) {
            isFloat = true;
            value.push_back(file.get());
            col++;
        }
        else if ((c == '+' || c == '-') && (value.back() == 'e' || value.back() == 'E')) {
            value.push_back(file.get());
            col++;
        }
        else break;
    }

    return {isFloat ? TokenType::Dec : TokenType::Int, value, line, startCol};
}

Token Lexer::readSymbol() {
    std::string value;
    int startCol = col;
    char ch = file.get();
    col++;

    value.push_back(ch);

    int next = file.peek();

    if (ch == '=' && next == '=') {
        value.push_back(file.get());
        col++;
    }
    else if (ch == '!' && next == '=') {
        value.push_back(file.get());
        col++;
    }
    else if (ch == '<' && next == '=') {
        value.push_back(file.get());
        col++;
    }
    else if (ch == '>' && next == '=') {
        value.push_back(file.get());
        col++;
    }

    return {TokenType::Symbol, value, line, startCol};
}

Token Lexer::readString() {
    std::string value;
    int startCol = col;

    char quote = file.get();
    col++;

    while (true) {
        int c = file.peek();
        if (c == EOF) {
            return {TokenType::Unknown, value, line, startCol};
        }

        char ch = file.get();
        col++;

        if (ch == quote){
            break;
        }

        if (ch == '\\') {
            int nextChar = file.get();
            col++;
            switch (nextChar) {
                case 'n': value.push_back('\n'); break;
                case 't': value.push_back('\t'); break;
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                default: value.push_back(static_cast<char>(nextChar)); break;
            }
        } else {
            value.push_back(ch);
        }

        if (ch == '\n') {
            line++;
            col = 1;
        }
    }

    return {TokenType::String, value, line, startCol};
}

Token Lexer::readChar() {
    std::string value;
    int startCol = col;

    char quote = file.get();
    col++;

    if (file.peek() == EOF) {
        return {TokenType::Unknown, "", line, startCol};
    }

    char ch = file.get();
    col++;

    if (ch == '\\') {
        int nextChar = file.get();
        col++;
        switch (nextChar) {
            case 'n': value.push_back('\n'); break;
            case 't': value.push_back('\t'); break;
            case '\'': value.push_back('\''); break;
            case '\\': value.push_back('\\'); break;
            default: value.push_back(static_cast<char>(nextChar)); break;
        }
    } else {
        value.push_back(ch);
    }

    if (file.get() != '\'') {
        return {TokenType::Unknown, value, line, startCol};
    }
    col++;

    return {TokenType::Char, value, line, startCol};
}

Token Lexer::nextToken() {
    skipIgnored();

    int c = file.peek();
    if (c == EOF){
        return {TokenType::EndOfFile, "", line, col};
    }

    if (std::isalpha(c) || c == '_') return readIdentifier();
    if (std::isdigit(c)) return readNumber();
    if (c == '\'') return readChar();
    if (c == '"') return readString();
    if (std::ispunct(c)) return readSymbol();

    char ch = file.get();
    return {TokenType::Unknown, std::string(1, ch), line, col++};
}
