#pragma once

#include <vector>
#include <memory>
#include "lexer.hpp"
#include "ast.hpp"

class Parser {
public:

    Parser(std::vector<Token>& tokens);
    std::vector<std::unique_ptr<ASTNode>> parse();

private:

    std::vector<Token>& tokens;
    size_t current = 0;

    Token& peek();
    Token& advance();
    bool match(const std::string& value);
    bool check(const std::string& value);

    std::unique_ptr<ASTNode> parseDeclaration();
    std::unique_ptr<ASTNode> parseAssignment();
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseBinaryExpr(std::unique_ptr<ASTNode> left, int prec);
    std::unique_ptr<ASTNode> parsePrimary();

};