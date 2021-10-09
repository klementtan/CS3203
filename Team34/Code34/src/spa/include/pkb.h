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

    using StatementNum = simple::ast::StatementNum;
    using StatementSet = std::unordered_set<StatementNum>;

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

        bool callsProcedure(const std::string& procname) const;
        bool callsProcedureTransitively(const std::string& procname) const;

        bool isCalledByProcedure(const std::string& procname) const;
        bool isTransitivelyCalledByProcedure(const std::string& procname) const;

        const std::unordered_set<std::string>& getAllCallers() const;
        const std::unordered_set<std::string>& getAllCalledProcedures() const;

        const std::unordered_set<std::string>& getAllTransitiveCallers() const;
        const std::unordered_set<std::string>& getAllTransitivelyCalledProcedures() const;

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

        StatementNum getStmtNum() const;

        bool hasFollower() const;
        bool isFollower() const;

        // a->doesFollow(b) <=> Follows(b, a) holds
        // a->isFollowedBy(b) <=> Follows(a, b) holds
        bool doesFollow(StatementNum id) const;
        bool isFollowedBy(StatementNum id) const;
        bool doesFollowTransitively(StatementNum id) const;
        bool isFollowedTransitivelyBy(StatementNum id) const;

        StatementNum getStmtDirectlyAfter() const;
        StatementNum getStmtDirectlyBefore() const;
        const StatementSet& getStmtsTransitivelyAfter() const;
        const StatementSet& getStmtsTransitivelyBefore() const;

        const simple::ast::Stmt* getAstStmt() const;

        bool usesVariable(const std::string& var_name) const;
        bool modifiesVariable(const std::string& var_name) const;

        const std::unordered_set<std::string>& getUsedVariables() const;
        const std::unordered_set<std::string>& getModifiedVariables() const;

        // a->isParentOf(b) <=> Parent(a, b) holds
        // a->isChildOf(b) <=> Parent(b, a) holds
        bool isParentOf(StatementNum id) const;
        bool isAncestorOf(StatementNum id) const;

        bool isChildOf(StatementNum id) const;
        bool isDescendantOf(StatementNum id) const;

        const StatementSet& getChildren() const;
        const StatementSet& getDescendants() const;
        const StatementSet& getParent() const;
        const StatementSet& getAncestors() const;

        const std::unordered_set<std::string>& getVariablesUsedInCondition() const;

    private:
        const simple::ast::Stmt* m_stmt = nullptr;

        std::unordered_set<std::string> m_uses {};
        std::unordered_set<std::string> m_modifies {};

        // only populated if the statement is an if or while.
        std::unordered_set<std::string> m_condition_uses {};

        StatementNum m_directly_before = 0;
        StatementNum m_directly_after = 0;

        // For a statement s, before stores all statements s1 for Follows*(s1, s) returns true,
        // after stores all statements s2 for Follows*(s, s1) returns true.
        StatementSet m_before {};
        StatementSet m_after {};

        // this must be a set because of the "unified" interface for relation evaluation,
        // even though there there can only be one parent.
        StatementSet m_parent {};
        StatementSet m_children {};

        StatementSet m_ancestors {};
        StatementSet m_descendants {};
    };

    struct Variable
    {
        Variable() = default;

        MOVE_ONLY_TYPE(Variable);

        friend struct DesignExtractor;

        StatementSet getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT ent) const;
        StatementSet getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT ent) const;

        const std::unordered_set<std::string>& getUsingProcNames() const;
        const std::unordered_set<std::string>& getModifyingProcNames() const;

    private:
        std::unordered_set<const Statement*> m_used_by {};
        std::unordered_set<const Statement*> m_modified_by {};

        std::unordered_set<std::string> m_used_by_procs {};
        std::unordered_set<std::string> m_modified_by_procs {};
    };

    struct CFG
    {
        CFG(int v);
        void addEdge(StatementNum stmt1, StatementNum stmt2);
        std::string getMatRep();
        void computeDistMat();
        bool isStatementNext(StatementNum stmt1, StatementNum stmt2);
        bool isStatementTransitivelyNext(StatementNum stmt1, StatementNum stmt2);
        StatementNum getNextStatement();
        const StatementSet& getTransitivielyNextStatements();
    private:
        int total_inst;
        // adjacency matrix for lengths of shortest paths between 2 instructions. i(row) is the source and j(col) is the destination.
        int** adj_mat;
    };

    struct ProgramKB
    {
        ProgramKB(std::unique_ptr<simple::ast::Program> program);
        ~ProgramKB();

        const Statement& getStatementAt(const StatementNum& id) const;
        Statement& getStatementAt(const StatementNum& id);

        const Procedure& getProcedureNamed(const std::string& name) const;
        Procedure& getProcedureNamed(const std::string& name);

        const Variable& getVariableNamed(const std::string& name) const;

        bool callsRelationExists() const;
        bool parentRelationExists() const;
        bool followsRelationExists() const;

        const simple::ast::Program* getProgram() const;

        const std::vector<Statement>& getAllStatements() const;
        const std::unordered_set<std::string>& getAllConstants() const;
        const std::unordered_map<std::string, Variable>& getAllVariables() const;
        const std::unordered_map<std::string, Procedure>& getAllProcedures() const;
        pkb::CFG* getCFG();

        void addConstant(std::string value);
        Procedure& addProcedure(const std::string& name, const simple::ast::Procedure* proc);

    private:
        std::unique_ptr<simple::ast::Program> m_program {};

        std::unordered_map<std::string, Procedure> m_procedures {};
        std::unordered_map<std::string, Variable> m_variables {};
        std::unordered_set<std::string> m_constants {};
        std::vector<Statement> m_statements {};
        std::unique_ptr<pkb::CFG> cfg {};

        bool m_follows_exists = false;
        bool m_parent_exists = false;
        bool m_calls_exists = false;

        friend struct DesignExtractor;
    };
}
