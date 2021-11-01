// PKB.h
// contains definitions for all the program knowledge base structures

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <set>

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

        const StatementSet& getCallStmts() const;

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

        StatementSet m_call_stmts {};
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

        const simple::ast::Procedure* getProc() const;

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

        // the containing procedure
        const simple::ast::Procedure* proc;
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

        const StatementSet& getReadStmts() const;
        const StatementSet& getPrintStmts() const;

    private:
        std::unordered_set<const Statement*> m_used_by {};
        std::unordered_set<const Statement*> m_modified_by {};

        std::unordered_set<std::string> m_used_by_procs {};
        std::unordered_set<std::string> m_modified_by_procs {};

        StatementSet m_read_stmts {};
        StatementSet m_print_stmts {};
    };

    struct CFG
    {
        CFG(size_t v);
        ~CFG();

        void addEdge(StatementNum stmt1, StatementNum stmt2);
        void addEdgeBip(StatementNum stmt1, StatementNum stmt2, size_t weight);
        void computeDistMat();
        void computeDistMatBip();
        std::string getMatRep(int i) const;
        bool nextRelationExists() const;
        bool affectsRelationExists() const;

        void addAssignStmtMapping(StatementNum stmt1, Statement* stmt2);
        void addModStmtMapping(StatementNum stmt1, Statement* stmt2);
        void addCallStmtMapping(StatementNum stmt1, Statement* stmt2);
        const Statement* getAssignStmtMapping(StatementNum id) const;
        const Statement* getCallStmtMapping(StatementNum id) const;
        const Statement* getModStmtMapping(StatementNum id) const;

        bool isStatementNext(StatementNum stmt1, StatementNum stmt2) const;
        bool isStatementTransitivelyNext(StatementNum stmt1, StatementNum stmt2) const;
        StatementSet getNextStatements(StatementNum id) const;
        StatementSet getTransitivelyNextStatements(StatementNum id) const;
        StatementSet getPreviousStatements(StatementNum id) const;
        StatementSet getTransitivelyPreviousStatements(StatementNum id) const;

        bool doesAffect(StatementNum stmt1, StatementNum stmt2) const;
        bool doesTransitivelyAffect(StatementNum stmt1, StatementNum stmt2) const;
        StatementSet getAffectedStatements(StatementNum id) const;
        StatementSet getTransitivelyAffectedStatements(StatementNum id) const;
        StatementSet getAffectingStatements(StatementNum id) const;
        StatementSet getTransitivelyAffectingStatements(StatementNum id) const;

    private:
        size_t total_inst;
        // adjacency matrix for lengths of shortest paths between 2 inst. i(row) is source and j(col) is destination.
        size_t** adj_mat;
        size_t** adj_mat_bip;

        bool m_next_exists = false;
        std::unordered_map<std::string, std::pair<StatementNum, std::vector<StatementNum>>> gates;
        std::unordered_map<StatementNum, StatementSet> adj_lst;
        std::unordered_map<StatementNum, std::vector<std::pair<StatementNum, size_t>>> adj_lst_bip;
        std::unordered_map<StatementNum, const Statement*> assign_stmts;
        std::unordered_map<StatementNum, const Statement*> mod_stmts;
        std::unordered_map<StatementNum, const Statement*> call_stmts;

        friend struct DesignExtractor;
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
        Variable& getVariableNamed(const std::string& name);

        bool nextRelationExists() const;
        bool callsRelationExists() const;
        bool parentRelationExists() const;
        bool followsRelationExists() const;
        bool affectsRelationExists() const;

        const simple::ast::Program* getProgram() const;

        const std::vector<Statement>& getAllStatements() const;
        const std::unordered_set<std::string>& getAllConstants() const;
        const std::unordered_map<std::string, Variable>& getAllVariables() const;
        const std::unordered_map<std::string, Procedure>& getAllProcedures() const;
        const pkb::CFG* getCFG() const;

        void addConstant(std::string value);
        Procedure& addProcedure(const std::string& name, const simple::ast::Procedure* proc);

        const StatementSet& getAllStatementsOfKind(pql::ast::DESIGN_ENT ent) const;

    private:
        std::unique_ptr<simple::ast::Program> m_program {};

        std::unordered_map<std::string, Procedure> m_procedures {};
        std::unordered_map<std::string, Variable> m_variables {};
        std::unordered_set<std::string> m_constants {};
        std::vector<Statement> m_statements {};
        std::unique_ptr<pkb::CFG> m_cfg {};

        std::unordered_map<pql::ast::DESIGN_ENT, StatementSet> m_stmt_kinds {};

        bool m_follows_exists = false;
        bool m_parent_exists = false;
        bool m_calls_exists = false;

        friend struct DesignExtractor;
    };
}
