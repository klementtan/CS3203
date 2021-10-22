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
        void assignStatementNumbers(const simple::ast::StmtList* list);

        struct TraversalState
        {
            // contains only ifs and whiles local to the current function
            std::vector<Statement*> local_stmt_stack {};

            // contains the procedures call stack
            std::vector<pkb::Procedure*> proc_stack {};
        };

        void processStmtList(const simple::ast::StmtList* list, const TraversalState& ts);
        void processExpr(const simple::ast::Expr* expr, Statement* stmt, const TraversalState& ts);

        void processUses(const std::string& var, Statement* stmt, const TraversalState& ts);
        void processModifies(const std::string& var, Statement* stmt, const TraversalState& ts);

        void processNextRelations();
        void processCFG(const simple::ast::StmtList* list, StatementNum last_checkpt);

        std::vector<Procedure*> processCallGraph();

    private:
        const simple::ast::Program* m_program {};
        std::unique_ptr<ProgramKB> m_pkb {};

        std::unordered_set<std::string> m_visited_procs {};
    };
}
