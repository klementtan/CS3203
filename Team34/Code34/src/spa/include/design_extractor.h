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
        void processCallGraph();

        void assignStatementNumbers(const simple::ast::StmtList* list);

        void processStmtList(const simple::ast::StmtList* list, const std::vector<const simple::ast::Stmt*>& stmt_stack,
            const std::vector<pkb::Procedure*>& proc_stack, const std::vector<Statement*>& call_stack);

        void processExpr(const simple::ast::Expr* expr, Statement* stmt,
            const std::vector<const simple::ast::Stmt*>& stmt_stack, const std::vector<pkb::Procedure*>& proc_stack,
            const std::vector<Statement*>& call_stack);

        void processUses(const std::string& var, Statement* stmt,
            const std::vector<const simple::ast::Stmt*>& stmt_stack, const std::vector<pkb::Procedure*>& proc_stack,
            const std::vector<Statement*>& call_stack);

        void processModifies(const std::string& var, Statement* stmt,
            const std::vector<const simple::ast::Stmt*>& stmt_stack, const std::vector<pkb::Procedure*>& proc_stack,
            const std::vector<Statement*>& call_stack);

    private:
        const simple::ast::Program* m_program {};
        std::unique_ptr<ProgramKB> m_pkb {};

        std::unordered_set<std::string> m_visited_procs {};
    };
}
