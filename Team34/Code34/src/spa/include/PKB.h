// PKB.h
// contains definitions for all the program knowledge base structures

#include <map>
#include <set>

#include "ast.h"

namespace pkb
{
    struct Procedure
    {
        std::string name;
        ast::Procedure* ast_proc = 0;

        std::set<std::string> uses;
        std::set<std::string> modifies;

        std::set<std::string> calls;
        std::set<std::string> called_by;
    };

    struct Variable
    {
        std::string name;

        // a procedure isn't really a statement, so we need to keep two
        // separate lists for this.
        std::set<ast::Stmt*> used_by;
        std::set<ast::Stmt*> modified_by;

        std::set<Procedure*> used_by_procs;
        std::set<Procedure*> modified_by_procs;
    };

    struct ProgramKB
    {
        // this also functions as a map from (stmt_number - 1) -> Stmt*,
        // and the Stmt knows its own number.
        std::vector<ast::Stmt*> statements;

        std::map<std::string, Variable> variables;
        std::map<std::string, Procedure> procedures;

        
        std::vector<ast::Stmt*> while_statements;
        std::vector<ast::Stmt*> if_statements;
    };

    ProgramKB* processProgram(ast::Program* prog);
}
