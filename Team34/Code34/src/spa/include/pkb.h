// PKB.h
// contains definitions for all the program knowledge base structures

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "pql/parser/ast.h"
#include "simple/ast.h"

namespace pkb
{
    struct DesignExtractor;

#define MOVE_ONLY_TYPE(TypeName)               \
    TypeName(TypeName&&) = default;            \
    TypeName& operator=(TypeName&&) = default; \
    TypeName(const TypeName&) = delete;        \
    TypeName& operator=(const TypeName&) = delete;

    struct Procedure
    {
        MOVE_ONLY_TYPE(Procedure);

        friend struct DesignExtractor;

        Procedure(const simple::ast::Procedure* ast_proc);

        bool usesVariable(const std::string& varname) const;
        bool modifiesVariable(const std::string& varname) const;

        const std::unordered_set<std::string>& getUsedVariables() const;
        const std::unordered_set<std::string>& getModifiedVariables() const;

        std::string getName() const;
        const simple::ast::Procedure* getAstProc() const;

    private:
        const simple::ast::Procedure* m_ast_proc = 0;

        std::unordered_set<std::string> m_uses {};
        std::unordered_set<std::string> m_modifies {};

        std::unordered_set<std::string> m_calls {};
        std::unordered_set<std::string> m_called_by {};

        std::unordered_set<std::string> m_calls_transitive {};
        std::unordered_set<std::string> m_called_by_transitive {};
    };

    struct Statement
    {
        MOVE_ONLY_TYPE(Statement);

        friend struct DesignExtractor;

        Statement(const simple::ast::Stmt* stmt);

        simple::ast::StatementNum getStmtNum() const;

        bool hasFollower() const;
        bool isFollower() const;

        bool doesFollow(simple::ast::StatementNum id) const;
        bool isFollowedBy(simple::ast::StatementNum id) const;
        bool doesFollowTransitively(simple::ast::StatementNum id) const;
        bool isFollowedTransitivelyBy(simple::ast::StatementNum id) const;

        simple::ast::StatementNum getDirectStmtAfter() const;
        simple::ast::StatementNum getDirectStmtBefore() const;
        const std::unordered_set<simple::ast::StatementNum>& getStmtsTransitivelyAfter() const;
        const std::unordered_set<simple::ast::StatementNum>& getStmtsTransitivelyBefore() const;

        const simple::ast::Stmt* getAstStmt() const;

        bool usesVariable(const std::string& var_name) const;
        bool modifiesVariable(const std::string& var_name) const;

        const std::unordered_set<std::string>& getUsedVariables() const;
        const std::unordered_set<std::string>& getModifiedVariables() const;

    private:
        const simple::ast::Stmt* m_stmt = nullptr;

        // stores uses and modifies information if the stmt is not a proc call
        std::unordered_set<std::string> m_uses {};
        std::unordered_set<std::string> m_modifies {};

        simple::ast::StatementNum m_directly_before = 0;
        simple::ast::StatementNum m_directly_after = 0;

        // For a statement s, before stores all statements s1 for Follows*(s1, s) returns true,
        // after stores all statements s2 for Follows*(s, s1) returns true.
        std::unordered_set<simple::ast::StatementNum> m_before {};
        std::unordered_set<simple::ast::StatementNum> m_after {};
    };

    struct Variable
    {
        Variable() = default;

        MOVE_ONLY_TYPE(Variable);

        friend struct DesignExtractor;

        std::unordered_set<simple::ast::StatementNum> getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT ent) const;
        std::unordered_set<simple::ast::StatementNum> getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT ent) const;

        std::unordered_set<std::string> getUsingProcNames() const;
        std::unordered_set<std::string> getModifyingProcNames() const;

    private:
        // a procedure isn't really a statement, so we need to keep two
        // separate lists for this.
        std::unordered_set<const Statement*> m_used_by {};
        std::unordered_set<const Statement*> m_modified_by {};

        std::unordered_set<const Procedure*> m_used_by_procs {};
        std::unordered_set<const Procedure*> m_modified_by_procs {};
    };

    struct ProgramKB
    {
        ProgramKB(std::unique_ptr<simple::ast::Program> program);
        ~ProgramKB();

        const Statement* getStatementAtIndex(simple::ast::StatementNum) const;
        Statement* getStatementAtIndex(simple::ast::StatementNum);

        const Procedure& getProcedureNamed(const std::string& name) const;
        Procedure& getProcedureNamed(const std::string& name);

        const Variable& getVariableNamed(const std::string& name) const;

        bool parentRelationExists() const;
        bool followsRelationExists() const;

#if 1
        bool isParent(simple::ast::StatementNum, simple::ast::StatementNum) const;
        bool isParentT(simple::ast::StatementNum, simple::ast::StatementNum) const;

        std::optional<simple::ast::StatementNum> getParentOf(simple::ast::StatementNum) const;
        std::unordered_set<simple::ast::StatementNum> getAncestorsOf(simple::ast::StatementNum) const;
        std::unordered_set<simple::ast::StatementNum> getChildrenOf(simple::ast::StatementNum) const;
        std::unordered_set<simple::ast::StatementNum> getDescendantsOf(simple::ast::StatementNum) const;
#endif

        const simple::ast::Program* getProgram() const;

        const std::vector<Statement>& getAllStatements() const;
        const std::unordered_set<std::string>& getAllConstants() const;
        const std::unordered_map<std::string, Variable>& getAllVariables() const;
        const std::unordered_map<std::string, Procedure>& getAllProcedures() const;

        void addConstant(std::string value);
        Procedure& addProcedure(const std::string& name, const simple::ast::Procedure* proc);

    private:
        std::unique_ptr<simple::ast::Program> m_program {};

        std::unordered_map<std::string, Procedure> m_procedures {};
        std::unordered_map<std::string, Variable> m_variables {};
        std::unordered_set<std::string> m_constants {};
        std::vector<Statement> m_statements {};

        std::unordered_map<simple::ast::StatementNum, simple::ast::StatementNum> m_direct_parents {};
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> m_ancestors {};
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>>
            m_direct_children {};
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> m_descendants {};

        bool m_follows_exists = false;
        bool m_parent_exists = false;

        friend struct DesignExtractor;
    };
}
