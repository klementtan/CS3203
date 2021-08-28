#include <assert.h>

#include <zpr.h>

#include "ast.h"
#include "PKB.h"

namespace pkb
{
    template <typename... Args>
    [[noreturn]] static void error(const char* fmt, Args&&... xs)
    {
		zpr::fprintln(stderr, "error: {}", zpr::fwd(fmt, static_cast<Args&&>(xs)...));
		exit(1);
    }


	// collection only entails numbering the statements
    static void collectStmtList(ProgramKB* pkb, ast::StmtList* list);
    static void collectStmt(ProgramKB* pkb, ast::Stmt* stmt, ast::StmtList* parent)
    {
		// the statement should not have been seen yet.
		assert(stmt->id == 0);

		stmt->parent_list = parent;
		stmt->id = pkb->statements.size() + 1;
		pkb->statements.push_back(stmt);

		if(auto i = dynamic_cast<ast::IfStmt*>(stmt); i)
        {
            pkb->if_statements.push_back(stmt);
			collectStmtList(pkb, &i->true_case);
			collectStmtList(pkb, &i->false_case);
            i->true_case.parent_statement = stmt;
            i->false_case.parent_statement = stmt;
		}
        else if(auto w = dynamic_cast<ast::WhileLoop*>(stmt); w)
        {
            pkb->while_statements.push_back(stmt);
			collectStmtList(pkb, &w->body);
            w->body.parent_statement = stmt;
		}
    }

    static void collectStmtList(ProgramKB* pkb, ast::StmtList* list)
    {
		for(const auto& stmt : list->statements)
			collectStmt(pkb, stmt, list);
    }


    ProgramKB* processProgram(ast::Program* program)
    {
		auto pkb = new ProgramKB();

		// do a first pass to number all the statements, set the
		// parent stmtlist, and collect all the procedures.
		for(const auto& proc : program->procedures)
		{
			collectStmtList(pkb, &proc->body);

			if(pkb->procedures.find(proc->name) != pkb->procedures.end())
				error("procedure '{}' is already defined", proc->name);

			pkb->procedures[proc->name].ast_proc = proc;
		}


		return pkb;
    }
}
