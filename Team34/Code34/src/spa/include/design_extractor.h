// design_extractor.h

#pragma once

#include "pkb.h"

namespace pkb
{
    struct DesignExtractor
    {
        DesignExtractor(std::unique_ptr<simple::ast::Program> program);
        std::unique_ptr<ProgramKB> run();

    private:
        void assignStatementNumbersAndProc(const simple::ast::StmtList* list, const simple::ast::Procedure* proc);

        struct TraversalState
        {
            std::vector<Statement*> local_stmt_stack {};
            pkb::Procedure* current_proc {};
        };

        void processFollowingForStmtList(const simple::ast::StmtList* list, TraversalState& ts);
        void processAncestryForStmt(Statement* stmt, TraversalState& ts);

        void processIfStmt(Statement* stmt, const simple::ast::IfStmt* i, TraversalState& ts);
        void processProcCall(Statement* stmt, const simple::ast::ProcCall* w, TraversalState& ts);
        void processWhileLoop(Statement* stmt, const simple::ast::WhileLoop* w, TraversalState& ts);

        void processStmtList(const simple::ast::StmtList* list, TraversalState& ts);
        void processExpr(const simple::ast::Expr* expr, Statement* stmt, const TraversalState& ts);

        void processUses(const std::string& var, Statement* stmt, const TraversalState& ts);
        void processModifies(const std::string& var, Statement* stmt, const TraversalState& ts);

        void processNextRelations();
        void processBipRelations();
        void processCFG(const simple::ast::StmtList* list, StatementNum last_checkpt);

        std::vector<Procedure*> processCallGraph();

    private:
        const simple::ast::Program* m_program {};
        std::unique_ptr<ProgramKB> m_pkb {};

        std::unordered_set<std::string> m_visited_procs {};
    };
}
