#ifndef CHRONOVYAN_AST_NODE_BASE_H
#define CHRONOVYAN_AST_NODE_BASE_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include "source_location.h"
#include "token.h"
#include "types.h"

namespace chronovyan {

// Forward declarations for visitor pattern
class Visitor;

// Forward declarations of all node types for interdependencies
// Base classes
class ASTNode;
class Expression;
class Statement;
class TemporalExpression;
class TemporalStatement;

// Possible types for node attributes
using AttributeValue = std::variant<
    std::nullptr_t,
    bool,
    int,
    double,
    std::string,
    std::vector<int>,
    std::vector<double>,
    std::vector<std::string>
>;

/**
 * @class ASTNode
 * @brief Base class for all abstract syntax tree nodes
 */
class ASTNode {
public:
    ASTNode(SourceLocation location);
    virtual ~ASTNode() = default;
    
    // Accept method for visitor pattern
    virtual void accept(Visitor& visitor) = 0;
    
    // Getter for source location
    const SourceLocation& getLocation() const;
    
    // Set and get attributes (used for storing metadata during compilation)
    void setAttribute(const std::string& key, AttributeValue value);
    bool hasAttribute(const std::string& key) const;
    const AttributeValue& getAttribute(const std::string& key) const;
    void removeAttribute(const std::string& key);

private:
    SourceLocation m_location;
    std::unordered_map<std::string, AttributeValue> m_attributes;
};

/**
 * @class Expression
 * @brief Base class for all expression nodes
 */
class Expression : public ASTNode {
public:
    Expression(SourceLocation location);
    virtual ~Expression() = default;
};

/**
 * @class Statement
 * @brief Base class for all statement nodes
 */
class Statement : public ASTNode {
public:
    Statement(SourceLocation location);
    virtual ~Statement() = default;
};

/**
 * @class TemporalExpression
 * @brief Base class for temporal expressions
 */
class TemporalExpression : public Expression {
public:
    TemporalExpression(SourceLocation location);
    virtual ~TemporalExpression() = default;
};

/**
 * @class TemporalStatement
 * @brief Base class for temporal statements
 */
class TemporalStatement : public Statement {
public:
    TemporalStatement(SourceLocation location);
    virtual ~TemporalStatement() = default;
};

} // namespace chronovyan

#endif // CHRONOVYAN_AST_NODE_BASE_H 