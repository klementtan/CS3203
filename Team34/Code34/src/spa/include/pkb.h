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

    struct Procedure
    {
        simple::ast::Procedure* ast_proc = 0;

        std::unordered_set<std::string> uses;
        std::unordered_set<std::string> modifies;

        std::unordered_set<std::string> calls;
        std::unordered_set<std::string> called_by;

        std::unordered_set<std::string> calls_transitive {};
        std::unordered_set<std::string> called_by_transitive {};
    };

    // decided to go with 3 data structures because this would allow all 3 kinds of
    // wildcard queries to be fast. I believe this would not cause consistency issues as
    // the pre-processing is once off.
    struct Statement
    {
        friend struct DesignExtractor;

        Statement(const simple::ast::Stmt* stmt);

        // cannot be copied
        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;

        Statement(Statement&&) = default;
        Statement& operator=(Statement&&) = default;

        bool hasFollower() const;
        bool isFollower() const;

        bool follows(simple::ast::StatementNum id) const;
        bool followedBy(simple::ast::StatementNum id) const;
        bool followsTransitively(simple::ast::StatementNum id) const;
        bool followedTransitivelyBy(simple::ast::StatementNum id) const;

        simple::ast::StatementNum getDirectFollower() const;
        simple::ast::StatementNum getDirectFollowee() const;
        const std::unordered_set<simple::ast::StatementNum>& getTransitiveFollowers() const;
        const std::unordered_set<simple::ast::StatementNum>& getTransitiveFollowees() const;

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
        // a procedure isn't really a statement, so we need to keep two
        // separate lists for this.
        std::unordered_set<const simple::ast::Stmt*> used_by;
        std::unordered_set<const simple::ast::Stmt*> modified_by;

        std::unordered_set<simple::ast::Procedure*> used_by_procs;
        std::unordered_set<simple::ast::Procedure*> modified_by_procs;
    };

    struct CallGraph
    {
        // adjacency graph for the edges. Proc A calls proc B gives edge (A, B).
        std::unordered_map<std::string, std::unordered_set<std::string>> adj;

        void addEdge(const std::string& a, std::string b);
        std::unordered_set<std::string>::iterator removeEdge(
            std::string a, std::string b, std::unordered_map<std::string, std::unordered_set<std::string>>* adj);

        bool dfs(std::string a, std::unordered_map<std::string, std::unordered_set<std::string>>* adj,
            std::unordered_set<std::string>* visited);
        bool cycleExists();
        std::string missingProc(const std::vector<std::unique_ptr<simple::ast::Procedure>>& procs);
    };

    struct ProgramKB
    {
        ProgramKB(std::unique_ptr<simple::ast::Program> program);
        ~ProgramKB();

        Statement* getStatementAtIndex(simple::ast::StatementNum);
        const Statement* getStatementAtIndex(simple::ast::StatementNum) const;

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

        // For queries of type Uses(3, "x")
        bool isUses(const simple::ast::StatementNum& stmt_num, const std::string& var) const;
        // For queries of type Uses("main", "x")
        bool isUses(const std::string& proc, const std::string& var) const;

        // For queries of type Uses("main", _)
        std::unordered_set<std::string> getUsesVars(const std::string& proc) const;
        // Returns the Statement numbers of queries of type Uses(a/r/s/p, "x")
        std::unordered_set<std::string> getUses(const pql::ast::DESIGN_ENT& type, const std::string& var) const;

        // For queries of type Modifies(3, "x")
        bool isModifies(const simple::ast::StatementNum& stmt_num, const std::string& var) const;
        // For queries of type Modifies("main", "x")
        bool isModifies(const std::string& proc, const std::string& var) const;
        // For queries of type Modifies("main", _)
        std::unordered_set<std::string> getModifiesVars(const std::string& proc) const;
        // Returns the Statement numbers of queries of type Modifies(a/pn/s/p, "x")
        std::unordered_set<std::string> getModifies(const pql::ast::DESIGN_ENT& type, const std::string& var) const;

        const Procedure& getProcedureNamed(const std::string& name) const;
        const simple::ast::Program* getProgram() const;

        const std::vector<Statement>& getAllStatements() const;
        const std::unordered_set<std::string>& getAllConstants() const;
        const std::unordered_map<std::string, Variable>& getAllVariables() const;
        const std::unordered_map<std::string, Procedure>& getAllProcedures() const;

        void addConstant(std::string value);
        Procedure& addProcedure(const std::string& name, simple::ast::Procedure* proc);


    private:
        std::unique_ptr<simple::ast::Program> m_program {};

        std::unordered_map<std::string, Procedure> m_procedures {};
        std::unordered_map<std::string, Variable> m_variables {};
        std::unordered_set<std::string> m_constants {};
        std::vector<Statement> m_statements {};

        std::unordered_map<simple::ast::StatementNum, simple::ast::StatementNum> m_direct_parents {};
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> m_ancestors {};
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> m_direct_children {};
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> m_descendants {};

        CallGraph m_call_graph {};

        bool m_follows_exists = false;
        bool m_parent_exists = false;

        friend struct DesignExtractor;
    };

    std::unique_ptr<ProgramKB> processProgram(std::unique_ptr<simple::ast::Program> prog);
}
