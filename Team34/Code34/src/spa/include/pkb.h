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
    struct Procedure
    {
        simple::ast::Procedure* ast_proc = 0;

        std::unordered_set<std::string> uses;
        std::unordered_set<std::string> modifies;

        std::unordered_set<std::string> calls;
        std::unordered_set<std::string> called_by;
    };

    // decided to go with 3 data structures because this would allow all 3 kinds of
    // wildcard queries to be fast. I believe this would not cause consistency issues as
    // the pre-processing is once off.
    struct Statement
    {
        simple::ast::Stmt* stmt = nullptr;

        // stores uses and modifies information if the stmt is not a proc call
        std::unordered_set<std::string> uses;
        std::unordered_set<std::string> modifies;
    };

    struct Variable
    {
        // a procedure isn't really a statement, so we need to keep two
        // separate lists for this.
        std::unordered_set<simple::ast::Stmt*> used_by;
        std::unordered_set<simple::ast::Stmt*> modified_by;

        std::unordered_set<simple::ast::Procedure*> used_by_procs;
        std::unordered_set<simple::ast::Procedure*> modified_by_procs;
    };

    struct Follows
    {
        simple::ast::StatementNum id = 0;
        simple::ast::StatementNum directly_before = 0;
        simple::ast::StatementNum directly_after = 0;

        // For a statement s, before stores all statements s1 for Follows*(s1, s) returns true,
        // after stores all statements s2 for Follows*(s, s1) returns true.
        std::unordered_set<simple::ast::StatementNum> before;
        std::unordered_set<simple::ast::StatementNum> after;
    };

    struct CallGraph
    {
        // adjacency graph for the edges. Proc A calls proc B gives edge (A, B).
        std::unordered_map<std::string, std::unordered_set<std::string>> adj;

        void addEdge(std::string& a, std::string b);
        std::unordered_set<std::string>::iterator removeEdge(
            std::string a, std::string b, std::unordered_map<std::string, std::unordered_set<std::string>>* adj);

        bool dfs(std::string a, std::unordered_map<std::string, std::unordered_set<std::string>>* adj,
            std::unordered_set<std::string>* visited);
        bool cycleExists();
        std::string missingProc(const std::vector<std::unique_ptr<simple::ast::Procedure>>& procs);
    };

    struct UsesModifies
    {
        ~UsesModifies();

        // this also functions as a unordered_map from (stmt_number - 1) -> Stmt*,
        // and the Stmt knows its own number.
        std::vector<Statement*> statements;

        std::unordered_map<std::string, Procedure> procedures;
        std::unordered_map<std::string, Variable> variables;

        // For queries of type Uses(3, "x")
        bool isUses(const simple::ast::StatementNum& stmt_num, const std::string& var);
        // For queries of type Uses("main", "x")
        bool isUses(const std::string& proc, const std::string& var);
        // For queries of type Uses(3, _)
        std::unordered_set<std::string> getUsesVars(const simple::ast::StatementNum& stmt_num);
        // For queries of type Uses("main", _)
        std::unordered_set<std::string> getUsesVars(const std::string& proc);
        // Returns the Statement numbers of queries of type Uses(a/r/s/p, "x")
        std::unordered_set<std::string> getUses(const pql::ast::DESIGN_ENT& type, const std::string& var);

        // For queries of type Modifies(3, "x")
        bool isModifies(const simple::ast::StatementNum& stmt_num, const std::string& var);
        // For queries of type Modifies("main", "x")
        bool isModifies(const std::string& proc, const std::string& var);
        // For queries of type Modifies(3, _)
        std::unordered_set<std::string> getModifiesVars(const simple::ast::StatementNum& stmt_num);
        // For queries of type Modifies("main", _)
        std::unordered_set<std::string> getModifiesVars(const std::string& var);
        // Returns the Statement numbers of queries of type Modifies(a/pn/s/p, "x")
        std::unordered_set<std::string> getModifies(const pql::ast::DESIGN_ENT& type, const std::string& var);
    };

    struct ProgramKB
    {
        ~ProgramKB();

        CallGraph proc_calls;

        UsesModifies uses_modifies;

        std::vector<Follows*> follows;

        Statement* getStatementAtIndex(simple::ast::StatementNum);

        bool followsRelationExists();
        bool parentRelationExists();

        bool isFollows(simple::ast::StatementNum fst, simple::ast::StatementNum snd);
        bool isFollowsT(simple::ast::StatementNum fst, simple::ast::StatementNum snd);
        Follows* getFollows(simple::ast::StatementNum fst);
        std::unordered_set<simple::ast::StatementNum> getFollowsTList(
            simple::ast::StatementNum fst, simple::ast::StatementNum snd);

        std::unordered_map<simple::ast::StatementNum, simple::ast::StatementNum> _direct_parents;
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> _ancestors;
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> _direct_children;
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> _descendants;

        bool isParent(simple::ast::StatementNum, simple::ast::StatementNum);
        bool isParentT(simple::ast::StatementNum, simple::ast::StatementNum);

        std::optional<simple::ast::StatementNum> getParentOf(simple::ast::StatementNum);
        std::unordered_set<simple::ast::StatementNum> getAncestorsOf(simple::ast::StatementNum);
        std::unordered_set<simple::ast::StatementNum> getChildrenOf(simple::ast::StatementNum);
        std::unordered_set<simple::ast::StatementNum> getDescendantsOf(simple::ast::StatementNum);

        std::unordered_set<std::string> _constants;
        std::unordered_set<std::string> getConstants();

        bool m_follows_exists = false;
        bool m_parent_exists = false;

        std::unique_ptr<simple::ast::Program> m_program {};
    };

    ProgramKB* processProgram(std::unique_ptr<simple::ast::Program> prog);
}
