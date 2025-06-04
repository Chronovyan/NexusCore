#ifndef CHRONOVYAN_AST_VISITOR_H
#define CHRONOVYAN_AST_VISITOR_H

namespace chronovyan {

// Forward declarations for all node types
class Identifier;
class LiteralExpression;
class BinaryExpression;
class UnaryExpression;
class GroupingExpression;
class VariableExpression;
class AssignExpression;
class CallExpression;
class GetExpression;
class SetExpression;
class ThisExpression;
class SuperExpression;
class ExpressionStatement;
class PrintStatement;
class VariableStatement;
class BlockStatement;
class IfStatement;
class WhileStatement;
class ForStatement;
class BreakStatement;
class ContinueStatement;
class ReturnStatement;
class FunctionStatement;
class ClassStatement;
class ImportStatement;
class TemporalLoopStatement;
class TemporalBranchStatement;
class TemporalRewindStatement;
class TemporalFastForwardStatement;
class ParallelExecutionStatement;
class SynchronizationPointStatement;
class TimelineStatement;
class TimepointStatement;
class ObservationStatement;
class ThreadStatement;
class ResourceStatement;
class TemporalContextStatement;
class TemporalMarkStatement;
class TemporalJumpStatement;
class TemporalWaitStatement;
class TemporalDilationStatement;
class TemporalCompressionStatement;
class TemporalExecutionPathStatement;
class TemporalQueueStatement;

/**
 * @class Visitor
 * @brief Interface for the visitor pattern to traverse the AST
 * 
 * This class defines the interface for the visitor pattern, which allows
 * operations to be performed on the AST nodes without modifying their classes.
 */
class Visitor {
public:
    virtual ~Visitor() = default;
    
    // Expression visitors
    virtual void visitIdentifier(Identifier* expr) = 0;
    virtual void visitLiteralExpression(LiteralExpression* expr) = 0;
    virtual void visitBinaryExpression(BinaryExpression* expr) = 0;
    virtual void visitUnaryExpression(UnaryExpression* expr) = 0;
    virtual void visitGroupingExpression(GroupingExpression* expr) = 0;
    virtual void visitVariableExpression(VariableExpression* expr) = 0;
    virtual void visitAssignExpression(AssignExpression* expr) = 0;
    virtual void visitCallExpression(CallExpression* expr) = 0;
    virtual void visitGetExpression(GetExpression* expr) = 0;
    virtual void visitSetExpression(SetExpression* expr) = 0;
    virtual void visitThisExpression(ThisExpression* expr) = 0;
    virtual void visitSuperExpression(SuperExpression* expr) = 0;
    
    // Statement visitors
    virtual void visitExpressionStatement(ExpressionStatement* stmt) = 0;
    virtual void visitPrintStatement(PrintStatement* stmt) = 0;
    virtual void visitVariableStatement(VariableStatement* stmt) = 0;
    virtual void visitBlockStatement(BlockStatement* stmt) = 0;
    virtual void visitIfStatement(IfStatement* stmt) = 0;
    virtual void visitWhileStatement(WhileStatement* stmt) = 0;
    virtual void visitForStatement(ForStatement* stmt) = 0;
    virtual void visitBreakStatement(BreakStatement* stmt) = 0;
    virtual void visitContinueStatement(ContinueStatement* stmt) = 0;
    virtual void visitReturnStatement(ReturnStatement* stmt) = 0;
    virtual void visitFunctionStatement(FunctionStatement* stmt) = 0;
    virtual void visitClassStatement(ClassStatement* stmt) = 0;
    virtual void visitImportStatement(ImportStatement* stmt) = 0;
    
    // Temporal node visitors
    virtual void visitTemporalLoopStatement(TemporalLoopStatement* stmt) = 0;
    virtual void visitTemporalBranchStatement(TemporalBranchStatement* stmt) = 0;
    virtual void visitTemporalRewindStatement(TemporalRewindStatement* stmt) = 0;
    virtual void visitTemporalFastForwardStatement(TemporalFastForwardStatement* stmt) = 0;
    virtual void visitParallelExecutionStatement(ParallelExecutionStatement* stmt) = 0;
    virtual void visitSynchronizationPointStatement(SynchronizationPointStatement* stmt) = 0;
    virtual void visitTimelineStatement(TimelineStatement* stmt) = 0;
    virtual void visitTimepointStatement(TimepointStatement* stmt) = 0;
    virtual void visitObservationStatement(ObservationStatement* stmt) = 0;
    virtual void visitThreadStatement(ThreadStatement* stmt) = 0;
    virtual void visitResourceStatement(ResourceStatement* stmt) = 0;
    virtual void visitTemporalContextStatement(TemporalContextStatement* stmt) = 0;
    virtual void visitTemporalMarkStatement(TemporalMarkStatement* stmt) = 0;
    virtual void visitTemporalJumpStatement(TemporalJumpStatement* stmt) = 0;
    virtual void visitTemporalWaitStatement(TemporalWaitStatement* stmt) = 0;
    virtual void visitTemporalDilationStatement(TemporalDilationStatement* stmt) = 0;
    virtual void visitTemporalCompressionStatement(TemporalCompressionStatement* stmt) = 0;
    virtual void visitTemporalExecutionPathStatement(TemporalExecutionPathStatement* stmt) = 0;
    virtual void visitTemporalQueueStatement(TemporalQueueStatement* stmt) = 0;
};

} // namespace chronovyan

#endif // CHRONOVYAN_AST_VISITOR_H 