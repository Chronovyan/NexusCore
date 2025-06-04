#ifndef CHRONOVYAN_AST_TEMPORAL_NODES_H
#define CHRONOVYAN_AST_TEMPORAL_NODES_H

#include "ast_node_base.h"
#include "ast_expressions.h"
#include "ast_statements.h"

namespace chronovyan {

// Forward declarations
class Visitor;

/**
 * @class TemporalLoopStatement
 * @brief Represents a temporal loop construct
 */
class TemporalLoopStatement : public TemporalStatement {
public:
    TemporalLoopStatement(
        SourceLocation location,
        std::unique_ptr<Expression> iterations,
        std::unique_ptr<BlockStatement> body
    );
    virtual ~TemporalLoopStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getIterations() const;
    BlockStatement* getBody() const;
    
private:
    std::unique_ptr<Expression> m_iterations;
    std::unique_ptr<BlockStatement> m_body;
};

/**
 * @class TemporalBranchStatement
 * @brief Represents a temporal branching point
 */
class TemporalBranchStatement : public TemporalStatement {
public:
    TemporalBranchStatement(
        SourceLocation location,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<BlockStatement> thenBranch,
        std::unique_ptr<BlockStatement> elseBranch
    );
    virtual ~TemporalBranchStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getCondition() const;
    BlockStatement* getThenBranch() const;
    BlockStatement* getElseBranch() const;
    
private:
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<BlockStatement> m_thenBranch;
    std::unique_ptr<BlockStatement> m_elseBranch;
};

/**
 * @class TemporalRewindStatement
 * @brief Represents a temporal rewind operation
 */
class TemporalRewindStatement : public TemporalStatement {
public:
    TemporalRewindStatement(
        SourceLocation location,
        std::unique_ptr<Expression> amount
    );
    virtual ~TemporalRewindStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getAmount() const;
    
private:
    std::unique_ptr<Expression> m_amount;
};

/**
 * @class TemporalFastForwardStatement
 * @brief Represents a temporal fast-forward operation
 */
class TemporalFastForwardStatement : public TemporalStatement {
public:
    TemporalFastForwardStatement(
        SourceLocation location,
        std::unique_ptr<Expression> amount
    );
    virtual ~TemporalFastForwardStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getAmount() const;
    
private:
    std::unique_ptr<Expression> m_amount;
};

/**
 * @class ParallelExecutionStatement
 * @brief Represents parallel execution of statements
 */
class ParallelExecutionStatement : public TemporalStatement {
public:
    ParallelExecutionStatement(
        SourceLocation location,
        std::vector<std::unique_ptr<BlockStatement>> blocks
    );
    virtual ~ParallelExecutionStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    const std::vector<std::unique_ptr<BlockStatement>>& getBlocks() const;
    
private:
    std::vector<std::unique_ptr<BlockStatement>> m_blocks;
};

/**
 * @class SynchronizationPointStatement
 * @brief Represents a synchronization point in temporal execution
 */
class SynchronizationPointStatement : public TemporalStatement {
public:
    SynchronizationPointStatement(
        SourceLocation location,
        const std::string& name
    );
    virtual ~SynchronizationPointStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    const std::string& getName() const;
    
private:
    std::string m_name;
};

/**
 * @class TimelineStatement
 * @brief Represents a timeline declaration
 */
class TimelineStatement : public TemporalStatement {
public:
    TimelineStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::unique_ptr<BlockStatement> body
    );
    virtual ~TimelineStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    BlockStatement* getBody() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::unique_ptr<BlockStatement> m_body;
};

/**
 * @class TimepointStatement
 * @brief Represents a timepoint declaration
 */
class TimepointStatement : public TemporalStatement {
public:
    TimepointStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::unique_ptr<Expression> time
    );
    virtual ~TimepointStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    Expression* getTime() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::unique_ptr<Expression> m_time;
};

/**
 * @class ObservationStatement
 * @brief Represents an observation of a timeline
 */
class ObservationStatement : public TemporalStatement {
public:
    ObservationStatement(
        SourceLocation location,
        std::unique_ptr<Expression> timeline,
        std::unique_ptr<Expression> timepoint
    );
    virtual ~ObservationStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getTimeline() const;
    Expression* getTimepoint() const;
    
private:
    std::unique_ptr<Expression> m_timeline;
    std::unique_ptr<Expression> m_timepoint;
};

/**
 * @class ThreadStatement
 * @brief Represents a thread declaration
 */
class ThreadStatement : public TemporalStatement {
public:
    ThreadStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::unique_ptr<BlockStatement> body
    );
    virtual ~ThreadStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    BlockStatement* getBody() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::unique_ptr<BlockStatement> m_body;
};

/**
 * @class ResourceStatement
 * @brief Represents a resource allocation
 */
class ResourceStatement : public TemporalStatement {
public:
    ResourceStatement(
        SourceLocation location,
        const std::string& resourceType,
        std::unique_ptr<Expression> amount,
        std::unique_ptr<Expression> duration,
        std::unique_ptr<BlockStatement> body
    );
    virtual ~ResourceStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    const std::string& getResourceType() const;
    Expression* getAmount() const;
    Expression* getDuration() const;
    BlockStatement* getBody() const;
    
private:
    std::string m_resourceType;
    std::unique_ptr<Expression> m_amount;
    std::unique_ptr<Expression> m_duration;
    std::unique_ptr<BlockStatement> m_body;
};

/**
 * @class TemporalContextStatement
 * @brief Represents a temporal context declaration
 */
class TemporalContextStatement : public TemporalStatement {
public:
    TemporalContextStatement(
        SourceLocation location,
        std::unique_ptr<Expression> context,
        std::unique_ptr<BlockStatement> body
    );
    virtual ~TemporalContextStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getContext() const;
    BlockStatement* getBody() const;
    
private:
    std::unique_ptr<Expression> m_context;
    std::unique_ptr<BlockStatement> m_body;
};

/**
 * @class TemporalMarkStatement
 * @brief Represents a temporal mark
 */
class TemporalMarkStatement : public TemporalStatement {
public:
    TemporalMarkStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name
    );
    virtual ~TemporalMarkStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    
private:
    std::unique_ptr<Identifier> m_name;
};

/**
 * @class TemporalJumpStatement
 * @brief Represents a temporal jump to a mark
 */
class TemporalJumpStatement : public TemporalStatement {
public:
    TemporalJumpStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> markName
    );
    virtual ~TemporalJumpStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getMarkName() const;
    
private:
    std::unique_ptr<Identifier> m_markName;
};

/**
 * @class TemporalWaitStatement
 * @brief Represents a temporal wait operation
 */
class TemporalWaitStatement : public TemporalStatement {
public:
    TemporalWaitStatement(
        SourceLocation location,
        std::unique_ptr<Expression> duration
    );
    virtual ~TemporalWaitStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getDuration() const;
    
private:
    std::unique_ptr<Expression> m_duration;
};

/**
 * @class TemporalDilationStatement
 * @brief Represents a temporal dilation operation
 */
class TemporalDilationStatement : public TemporalStatement {
public:
    TemporalDilationStatement(
        SourceLocation location,
        std::unique_ptr<Expression> factor,
        std::unique_ptr<BlockStatement> body
    );
    virtual ~TemporalDilationStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getFactor() const;
    BlockStatement* getBody() const;
    
private:
    std::unique_ptr<Expression> m_factor;
    std::unique_ptr<BlockStatement> m_body;
};

/**
 * @class TemporalCompressionStatement
 * @brief Represents a temporal compression operation
 */
class TemporalCompressionStatement : public TemporalStatement {
public:
    TemporalCompressionStatement(
        SourceLocation location,
        std::unique_ptr<Expression> factor,
        std::unique_ptr<BlockStatement> body
    );
    virtual ~TemporalCompressionStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Expression* getFactor() const;
    BlockStatement* getBody() const;
    
private:
    std::unique_ptr<Expression> m_factor;
    std::unique_ptr<BlockStatement> m_body;
};

/**
 * @class TemporalExecutionPathStatement
 * @brief Represents a temporal execution path
 */
class TemporalExecutionPathStatement : public TemporalStatement {
public:
    TemporalExecutionPathStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::unique_ptr<BlockStatement> body
    );
    virtual ~TemporalExecutionPathStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    BlockStatement* getBody() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::unique_ptr<BlockStatement> m_body;
};

/**
 * @class TemporalQueueStatement
 * @brief Represents a temporal operation queue
 */
class TemporalQueueStatement : public TemporalStatement {
public:
    TemporalQueueStatement(
        SourceLocation location,
        std::unique_ptr<Identifier> name,
        std::vector<std::unique_ptr<Statement>> operations
    );
    virtual ~TemporalQueueStatement() = default;
    
    void accept(Visitor& visitor) override;
    
    Identifier* getName() const;
    const std::vector<std::unique_ptr<Statement>>& getOperations() const;
    
private:
    std::unique_ptr<Identifier> m_name;
    std::vector<std::unique_ptr<Statement>> m_operations;
};

} // namespace chronovyan

#endif // CHRONOVYAN_AST_TEMPORAL_NODES_H 