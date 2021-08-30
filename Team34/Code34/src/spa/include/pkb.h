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

        std::vector<ast::StatementNum> before;
        std::vector<ast::StatementNum> after;
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
    };

    ProgramKB* processProgram(ast::Program* prog);
}
