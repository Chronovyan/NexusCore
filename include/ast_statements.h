#ifndef CHRONOVYAN_AST_STATEMENTS_H
#define CHRONOVYAN_AST_STATEMENTS_H

#include "ast_node_base.h"
#include "ast_expressions.h"

namespace chronovyan {

// Forward declarations
class Visitor;

/**
 * @class ExpressionStatement
 * @brief Represents an expression used as a statement
 */
class ExpressionStatement : public Statement {
public:
    ExpressionStatement(
        SourceLocation location,
        std::unique_ptr<Expression> expression
    );
    virtual ~ExpressionStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getExpression() const;
    
private:
    std::unique_ptr<Expression> m_expression;
};

/**
 * @class PrintStatement
 * @brief Represents a print statement
 */
class PrintStatement : public Statement {
public:
    PrintStatement(
        SourceLocation location,
        std::unique_ptr<Expression> expression
    );
    virtual ~PrintStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getExpression() const;
    
private:
    std::unique_ptr<Expression> m_expression;
};

/**
 * @class VariableStatement
 * @brief Represents a variable declaration
 */
class VariableStatement : public Statement {
public:
    VariableStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::unique_ptr<Expression> initializer = nullptr,
        const std::string& typeName = ""
    );
    virtual ~VariableStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    Expression* getInitializer() const;
    const std::string& getTypeName() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::unique_ptr<Expression> m_initializer;
    std::string m_typeName;
};

/**
 * @class BlockStatement
 * @brief Represents a block of statements
 */
class BlockStatement : public Statement {
public:
    BlockStatement(
        SourceLocation location,
        std::vector<std::unique_ptr<Statement>> statements
    );
    virtual ~BlockStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    const std::vector<std::unique_ptr<Statement>>& getStatements() const;
    
private:
    std::vector<std::unique_ptr<Statement>> m_statements;
};

/**
 * @class IfStatement
 * @brief Represents an if statement
 */
class IfStatement : public Statement {
public:
    IfStatement(
        SourceLocation location,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Statement> thenBranch,
        std::unique_ptr<Statement> elseBranch = nullptr
    );
    virtual ~IfStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getCondition() const;
    Statement* getThenBranch() const;
    Statement* getElseBranch() const;
    
private:
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<Statement> m_thenBranch;
    std::unique_ptr<Statement> m_elseBranch;
};

/**
 * @class WhileStatement
 * @brief Represents a while loop
 */
class WhileStatement : public Statement {
public:
    WhileStatement(
        SourceLocation location,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Statement> body
    );
    virtual ~WhileStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getCondition() const;
    Statement* getBody() const;
    
private:
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<Statement> m_body;
};

/**
 * @class ForStatement
 * @brief Represents a for loop
 */
class ForStatement : public Statement {
public:
    ForStatement(
        SourceLocation location,
        std::unique_ptr<Statement> initializer,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Expression> increment,
        std::unique_ptr<Statement> body
    );
    virtual ~ForStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Statement* getInitializer() const;
    Expression* getCondition() const;
    Expression* getIncrement() const;
    Statement* getBody() const;
    
private:
    std::unique_ptr<Statement> m_initializer;
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<Expression> m_increment;
    std::unique_ptr<Statement> m_body;
};

/**
 * @class BreakStatement
 * @brief Represents a break statement
 */
class BreakStatement : public Statement {
public:
    BreakStatement(SourceLocation location);
    virtual ~BreakStatement() = default;
    
    void accept(Visitor& visitor) override;
};

/**
 * @class ContinueStatement
 * @brief Represents a continue statement
 */
class ContinueStatement : public Statement {
public:
    ContinueStatement(SourceLocation location);
    virtual ~ContinueStatement() = default;
    
    void accept(Visitor& visitor) override;
};

/**
 * @class ReturnStatement
 * @brief Represents a return statement
 */
class ReturnStatement : public Statement {
public:
    ReturnStatement(
        SourceLocation location,
        std::unique_ptr<Expression> value = nullptr
    );
    virtual ~ReturnStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getValue() const;
    
private:
    std::unique_ptr<Expression> m_value;
};

/**
 * @class FunctionStatement
 * @brief Represents a function declaration
 */
class FunctionStatement : public Statement {
public:
    FunctionStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::vector<std::unique_ptr<Identifier>> parameters,
        std::vector<std::string> paramTypes,
        std::unique_ptr<BlockStatement> body,
        const std::string& returnType = ""
    );
    virtual ~FunctionStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    const std::vector<std::unique_ptr<Identifier>>& getParameters() const;
    const std::vector<std::string>& getParamTypes() const;
    BlockStatement* getBody() const;
    const std::string& getReturnType() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::vector<std::unique_ptr<Identifier>> m_parameters;
    std::vector<std::string> m_paramTypes;
    std::unique_ptr<BlockStatement> m_body;
    std::string m_returnType;
};

/**
 * @class ClassStatement
 * @brief Represents a class declaration
 */
class ClassStatement : public Statement {
public:
    ClassStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::unique_ptr<Identifier> superclass,
        std::vector<std::unique_ptr<FunctionStatement>> methods
    );
    virtual ~ClassStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    Identifier* getSuperclass() const;
    const std::vector<std::unique_ptr<FunctionStatement>>& getMethods() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::unique_ptr<Identifier> m_superclass;
    std::vector<std::unique_ptr<FunctionStatement>> m_methods;
};

/**
 * @class ImportStatement
 * @brief Represents an import statement
 */
class ImportStatement : public Statement {
public:
    ImportStatement(
        SourceLocation location,
        const std::string& path,
        std::vector<std::unique_ptr<Identifier>> imports
    );
    virtual ~ImportStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    const std::string& getPath() const;
    const std::vector<std::unique_ptr<Identifier>>& getImports() const;
    
private:
    std::string m_path;
    std::vector<std::unique_ptr<Identifier>> m_imports;
};

} // namespace chronovyan

#endif // CHRONOVYAN_AST_STATEMENTS_H 