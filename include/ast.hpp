#pragma once

#include <string>
#include <memory>


struct ASTNode {
    virtual ~ASTNode() = default;
};


struct LiteralNode : ASTNode {
    std::string value;
    LiteralNode(const std::string& val) : value(val) {}
};

struct IdentifierNode : ASTNode {
    std::string name;
    IdentifierNode(const std::string& n) : name(n) {}
};

struct BinaryExprNode : ASTNode {
    std::unique_ptr<ASTNode> left;
    std::string op;
    std::unique_ptr<ASTNode> right;

    BinaryExprNode(std::unique_ptr<ASTNode> l, const std::string& o, std::unique_ptr<ASTNode> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

struct AssignmentNode : ASTNode {
    std::string identifier;
    std::unique_ptr<ASTNode> expression;

    AssignmentNode(const std::string& id, std::unique_ptr<ASTNode> expr)
        : identifier(id), expression(std::move(expr)) {}
};

struct DeclarationNode : ASTNode {
    std::string type;
    std::string identifier;
    std::unique_ptr<ASTNode> expression;

    DeclarationNode(const std::string& t, const std::string& id, std::unique_ptr<ASTNode> expr)
        : type(t), identifier(id), expression(std::move(expr)) {}
};