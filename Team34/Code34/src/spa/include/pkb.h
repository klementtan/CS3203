// PKB.h
// contains definitions for all the program knowledge base structures

#include <unordered_map>
#include <unordered_set>

#include "ast.h"

namespace pkb
{
    struct Procedure
    {
        std::string name;
        ast::Procedure* ast_proc = 0;

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
        std::unordered_set<ast::Stmt*> used_by;
        std::unordered_set<ast::Stmt*> modified_by;

        std::unordered_set<Procedure*> used_by_procs;
        std::unordered_set<Procedure*> modified_by_procs;
    };

    struct Follows
    {
        ast::StatementNum id = 0;
        ast::StatementNum directly_before = 0;
        ast::StatementNum directly_after = 0;

        // For a statement s, before stores all statements s1 for Follows*(s1, s) returns true,
        // after stores all statements s2 for Follows*(s, s1) returns true.
        std::unordered_set<ast::StatementNum> before;
        std::unordered_set<ast::StatementNum> after;
    };


    struct ProgramKB
    {
        // this also functions as a unordered_map from (stmt_number - 1) -> Stmt*,
        // and the Stmt knows its own number.
        std::vector<ast::Stmt*> statements;

        std::unordered_map<std::string, Variable> variables;
        std::unordered_map<std::string, Procedure> procedures;


        std::vector<ast::Stmt*> while_statements;
        std::vector<ast::Stmt*> if_statements;

        std::vector<Follows*> follows;

        bool isFollows(ast::StatementNum fst, ast::StatementNum snd);
        bool isFollowsT(ast::StatementNum fst, ast::StatementNum snd);
        std::unordered_set<ast::StatementNum> getFollowsTList(ast::StatementNum fst, ast::StatementNum snd);
    };

    ProgramKB* processProgram(ast::Program* prog);
}
