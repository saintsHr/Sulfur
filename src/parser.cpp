#include "parser.hpp"
#include "error.hpp"
#include "main.hpp"

const std::vector<std::string> decs = {
    "int",
    "dec",
    "string",
    "char",
    "bool",
};

Parser::Parser(std::vector<Token>& toks) : tokens(toks), current(0) {}

std::vector<std::unique_ptr<ASTNode>> Parser::parse(){
    std::vector<std::unique_ptr<ASTNode>> nodes;

    while (current < tokens.size()){
        Token& tok = peek();

        for (int i = 0; i < sizeof(decs); i++){
            if (tok.value == decs[i]){
                nodes.push_back(parseDeclaration());
            } else if (tok.type == TokenType::Identifier){
                nodes.push_back(parseAssignment());
            } else {
                error err;
                err.code = 5;
                err.file = source;
                err.source = "Parser";
                err.line = tok.line;
                err.col = tok.col;
                err.msg = "Unexpected token: " + tok.value;
                printErr(err);
                advance();
            }
        }
    }

    return nodes;
}

Token& Parser::peek() {
    return tokens[current];
}

Token& Parser::advance() {
    if (current < tokens.size()) current++;
    return tokens[current - 1];
}

bool Parser::match(const std::string& value) {
    if (peek().value == value) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(const std::string& value) {
    return peek().value == value;
}

std::unique_ptr<ASTNode> Parser::parseDeclaration() {
    std::string type = advance().value;

    if (peek().type != TokenType::Identifier) {
        error err;
        err.code = 6;
        err.file = source;
        err.source = "Parser";
        err.line = peek().line;
        err.col = peek().col;
        err.msg = "Expected identifier after type at line " + peek().line;
        printErr(err);
        return nullptr;
    }

    std::string id = advance().value;

    std::unique_ptr<ASTNode> expr = nullptr;

    if (match("=")) {
        expr = parseExpression();
    }

    return std::make_unique<DeclarationNode>(type, id, std::move(expr));
}

std::unique_ptr<ASTNode> Parser::parseAssignment() {
    std::string id = advance().value;

    if (!match("=")) {
        error err;
        err.code = 7;
        err.file = source;
        err.source = "Parser";
        err.line = peek().line;
        err.col = peek().col;
        err.msg = "Expected '=' after identifier at line " + peek().line;
        printErr(err);
        return nullptr;
    }

    std::unique_ptr<ASTNode> expr = parseExpression();

    return std::make_unique<AssignmentNode>(id, std::move(expr));
}

std::unique_ptr<ASTNode> Parser::parseExpression() {
    return parsePrimary();
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    Token& tok = peek();

    if (tok.type == TokenType::Dec || tok.type == TokenType::Int || tok.type == TokenType::String || tok.type == TokenType::Char) {
        advance();
        return std::make_unique<LiteralNode>(tok.value);
    }

    if (tok.type == TokenType::Identifier) {
        advance();
        return std::make_unique<IdentifierNode>(tok.value);
    }

    error err;
    err.code = 8;
    err.file = source;
    err.source = "Parser";
    err.line = peek().line;
    err.col = peek().col;
    err.msg = "Unexpected token in expression: " + tok.value + " at line " + std::to_string(tok.line) + ", col " + std::to_string(tok.col);
    printErr(err);
    std::cerr << "Unexpected token in expression: " << tok.value << " at line " << tok.line << ", col " << tok.col << std::endl;
    advance();
    return nullptr;
}