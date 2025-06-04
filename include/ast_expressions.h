#ifndef CHRONOVYAN_AST_EXPRESSIONS_H
#define CHRONOVYAN_AST_EXPRESSIONS_H

#include "ast_node_base.h"
#include "token.h"

namespace chronovyan {

// Forward declarations
class BlockStatement;
class Visitor;

/**
 * @class Identifier
 * @brief Represents an identifier in the AST
 */
class Identifier : public Expression {
public:
    Identifier(SourceLocation location, const std::string& name);
    virtual ~Identifier() = default;
    
    void accept(Visitor& visitor) override;
    
    const std::string& getName() const;
    
private:
    std::string m_name;
};

/**
 * @class LiteralExpression
 * @brief Represents a literal value in the AST
 */
class LiteralExpression : public Expression {
public:
    LiteralExpression(SourceLocation location, std::nullptr_t value);
    LiteralExpression(SourceLocation location, bool value);
    LiteralExpression(SourceLocation location, double value);
    LiteralExpression(SourceLocation location, int value);
    LiteralExpression(SourceLocation location, const std::string& value);
    virtual ~LiteralExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    // Getters for literal value
    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    
    std::nullptr_t getNullValue() const;
    bool getBoolValue() const;
    double getNumberValue() const;
    const std::string& getStringValue() const;
    
private:
    enum class LiteralType {
        Null,
        Bool,
        Number,
        String
    };
    
    LiteralType m_type;
    std::variant<std::nullptr_t, bool, double, std::string> m_value;
};

/**
 * @class BinaryExpression
 * @brief Represents a binary operation in the AST
 */
class BinaryExpression : public Expression {
public:
    BinaryExpression(
        SourceLocation location,
        std::unique_ptr<Expression> left,
        Token op,
        std::unique_ptr<Expression> right
    );
    virtual ~BinaryExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getLeft() const;
    const Token& getOperator() const;
    Expression* getRight() const;
    
private:
    std::unique_ptr<Expression> m_left;
    Token m_operator;
    std::unique_ptr<Expression> m_right;
};

/**
 * @class UnaryExpression
 * @brief Represents a unary operation in the AST
 */
class UnaryExpression : public Expression {
public:
    UnaryExpression(
        SourceLocation location,
        Token op,
        std::unique_ptr<Expression> right
    );
    virtual ~UnaryExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    const Token& getOperator() const;
    Expression* getRight() const;
    
private:
    Token m_operator;
    std::unique_ptr<Expression> m_right;
};

/**
 * @class GroupingExpression
 * @brief Represents a parenthesized expression in the AST
 */
class GroupingExpression : public Expression {
public:
    GroupingExpression(
        SourceLocation location,
        std::unique_ptr<Expression> expression
    );
    virtual ~GroupingExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getExpression() const;
    
private:
    std::unique_ptr<Expression> m_expression;
};

/**
 * @class VariableExpression
 * @brief Represents a variable reference in the AST
 */
class VariableExpression : public Expression {
public:
    VariableExpression(
        SourceLocation location,
        std::unique_ptr<Identifier> name
    );
    virtual ~VariableExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    
private:
    std::unique_ptr<Identifier> m_name;
};

/**
 * @class AssignExpression
 * @brief Represents a variable assignment in the AST
 */
class AssignExpression : public Expression {
public:
    AssignExpression(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::unique_ptr<Expression> value
    );
    virtual ~AssignExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    Expression* getValue() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::unique_ptr<Expression> m_value;
};

/**
 * @class CallExpression
 * @brief Represents a function call in the AST
 */
class CallExpression : public Expression {
public:
    CallExpression(
        SourceLocation location,
        std::unique_ptr<Expression> callee,
        std::vector<std::unique_ptr<Expression>> arguments
    );
    virtual ~CallExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getCallee() const;
    const std::vector<std::unique_ptr<Expression>>& getArguments() const;
    
private:
    std::unique_ptr<Expression> m_callee;
    std::vector<std::unique_ptr<Expression>> m_arguments;
};

/**
 * @class GetExpression
 * @brief Represents a property access in the AST
 */
class GetExpression : public Expression {
public:
    GetExpression(
        SourceLocation location,
        std::unique_ptr<Expression> object,
        std::unique_ptr<Identifier> name
    );
    virtual ~GetExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getObject() const;
    Identifier* getName() const;
    
private:
    std::unique_ptr<Expression> m_object;
    std::unique_ptr<Identifier> m_name;
};

/**
 * @class SetExpression
 * @brief Represents a property assignment in the AST
 */
class SetExpression : public Expression {
public:
    SetExpression(
        SourceLocation location,
        std::unique_ptr<Expression> object,
        std::unique_ptr<Identifier> name,
        std::unique_ptr<Expression> value
    );
    virtual ~SetExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getObject() const;
    Identifier* getName() const;
    Expression* getValue() const;
    
private:
    std::unique_ptr<Expression> m_object;
    std::unique_ptr<Identifier> m_name;
    std::unique_ptr<Expression> m_value;
};

/**
 * @class ThisExpression
 * @brief Represents a "this" reference in the AST
 */
class ThisExpression : public Expression {
public:
    ThisExpression(SourceLocation location);
    virtual ~ThisExpression() = default;
    
    void accept(Visitor& visitor) override;
};

/**
 * @class SuperExpression
 * @brief Represents a "super" reference in the AST
 */
class SuperExpression : public Expression {
public:
    SuperExpression(
        SourceLocation location,
        std::unique_ptr<Identifier> method
    );
    virtual ~SuperExpression() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getMethod() const;
    
private:
    std::unique_ptr<Identifier> m_method;
};

} // namespace chronovyan

#endif // CHRONOVYAN_AST_EXPRESSIONS_H 