// PKB.h
// contains definitions for all the program knowledge base structures

#include <unordered_map>
#include <unordered_set>
#include <zst.h>

#include "ast.h"

namespace pkb
{
    template <typename T>
    using Result = zst::Result<T, std::string>;

    struct Procedure
    {
        std::string name;
        simple::ast::Procedure* ast_proc = 0;

        std::unordered_set<std::string> uses;
        std::unordered_set<std::string> modifies;

        std::unordered_set<std::string> calls;
        std::unordered_set<std::string> called_by;
    };

    struct Variable
    {
        std::string name;

        // a procedure isn't really a statement, so we need to keep two
        // separate lists for this.
        std::unordered_set<simple::ast::Stmt*> used_by;
        std::unordered_set<simple::ast::Stmt*> modified_by;

        std::unordered_set<Procedure*> used_by_procs;
        std::unordered_set<Procedure*> modified_by_procs;
    };

    struct SymbolTable
    {
        std::unordered_map<std::string, simple::ast::VarRef*> _vars;

        simple::ast::VarRef* getVar(const std::string&);
        bool hasVar(const std::string&);
        void setVar(const std::string&, simple::ast::VarRef*);

        std::unordered_map<std::string, simple::ast::Procedure*> _procs;

        simple::ast::Procedure* getProc(const std::string&);
        bool hasProc(const std::string&);
        void setProc(const std::string&, simple::ast::Procedure*);

        void populate(simple::ast::Program*);
        void populate(const simple::ast::StmtList&);
        void populate(simple::ast::Expr*);
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
        std::string missingProc(std::unordered_map<std::string, Procedure>* procs);
    };

    struct ProgramKB
    {
        // this also functions as a unordered_map from (stmt_number - 1) -> Stmt*,
        // and the Stmt knows its own number.
        std::vector<simple::ast::Stmt*> statements;

        std::unordered_map<std::string, Variable> variables;
        std::unordered_map<std::string, Procedure> procedures;


        std::vector<simple::ast::Stmt*> while_statements;
        std::vector<simple::ast::Stmt*> if_statements;

        CallGraph proc_calls;

        std::vector<Follows*> follows;

        bool isFollows(simple::ast::StatementNum fst, simple::ast::StatementNum snd);
        bool isFollowsT(simple::ast::StatementNum fst, simple::ast::StatementNum snd);
        std::unordered_set<simple::ast::StatementNum> getFollowsTList(
            simple::ast::StatementNum fst, simple::ast::StatementNum snd);

        std::unordered_map<simple::ast::StatementNum, simple::ast::StatementNum> _direct_parents;
        std::unordered_map<simple::ast::StatementNum, std::unordered_set<simple::ast::StatementNum>> _ancestors;

        bool isParent(simple::ast::StatementNum, simple::ast::StatementNum);
        bool isParentT(simple::ast::StatementNum, simple::ast::StatementNum);
        std::unordered_set<simple::ast::StatementNum> getAncestorsOf(simple::ast::StatementNum);
    };

    Result<ProgramKB*> processProgram(simple::ast::Program* prog);
}
