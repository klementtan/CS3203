#include <assert.h>

#include <zpr.h>
#include <zst.h>

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
			collectStmtList(pkb, &i->true_case);
			collectStmtList(pkb, &i->false_case);
		}
        else if(auto w = dynamic_cast<ast::WhileLoop*>(stmt); w)
		{
			collectStmtList(pkb, &w->body);
		}
	}

	static void collectStmtList(ProgramKB* pkb, ast::StmtList* list)
	{
		for(const auto& stmt : list->statements)
			collectStmt(pkb, stmt, list);
	}


	static void processExpr(ProgramKB* pkb, ast::Expr* expr, ast::Stmt* parent_stmt, Procedure* parent_proc)
	{
        if(auto vr = dynamic_cast<ast::VarRef*>(expr))
		{
			pkb->variables[vr->name].used_by.insert(parent_stmt);
			pkb->variables[vr->name].used_by_procs.insert(parent_proc);

			pkb->procedures[parent_proc->name].uses.insert(vr->name);
		}
		else if(auto cc = dynamic_cast<ast::Constant*>(expr))
		{
			// do nothing
		}
        else if(auto bo = dynamic_cast<ast::BinaryOp*>(expr))
		{
			processExpr(pkb, bo->lhs, parent_stmt, parent_proc);
		}
        else if(auto uo = dynamic_cast<ast::UnaryOp*>(expr))
		{
			processExpr(pkb, uo->expr, parent_stmt, parent_proc);
		}
		else
		{
			error("unknown expression type");
		}
	}

	static void processStmtList(ProgramKB* pkb, ast::StmtList* list, Procedure* parent_proc);
    static void processStmt(ProgramKB* pkb, ast::Stmt* stmt, Procedure* parent_proc)
	{
        if(auto i = dynamic_cast<ast::IfStmt*>(stmt))
		{
			processStmtList(pkb, &i->true_case, parent_proc);
			processStmtList(pkb, &i->false_case, parent_proc);

			processExpr(pkb, i->condition, i, parent_proc);
		}
        else if(auto w = dynamic_cast<ast::WhileLoop*>(stmt))
		{
			processStmtList(pkb, &w->body, parent_proc);

			processExpr(pkb, w->condition, w, parent_proc);
		}
        else if(auto a = dynamic_cast<ast::AssignStmt*>(stmt))
		{
			pkb->variables[a->lhs].modified_by.insert(stmt);
			pkb->variables[a->lhs].modified_by_procs.insert(parent_proc);

			pkb->procedures[parent_proc->name].modifies.insert(a->lhs);
		}
        else if(auto r = dynamic_cast<ast::ReadStmt*>(stmt))
		{
			pkb->variables[r->var_name].modified_by.insert(stmt);
			pkb->variables[r->var_name].modified_by_procs.insert(parent_proc);

			pkb->procedures[parent_proc->name].modifies.insert(r->var_name);
		}
        else if(auto p = dynamic_cast<ast::PrintStmt*>(stmt))
		{
			pkb->variables[p->var_name].used_by.insert(stmt);
			pkb->variables[p->var_name].used_by_procs.insert(parent_proc);

			pkb->procedures[parent_proc->name].uses.insert(p->var_name);
		}
        else if(auto c = dynamic_cast<ast::ProcCall*>(stmt))
		{
			pkb->procedures[parent_proc->name].calls.insert(c->proc_name);
			pkb->procedures[c->proc_name].called_by.insert(parent_proc->name);
		}
		else
		{
			error("unknown statement type");
		}
	}

	static void processStmtList(ProgramKB* pkb, ast::StmtList* list, Procedure* parent_proc)
	{
		for(const auto& stmt : list->statements)
			processStmt(pkb, stmt, parent_proc);
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

		// second pass to find the relationships
		for(auto& [ _, proc ] : pkb->procedures)
			processStmtList(pkb, &proc.ast_proc->body, &proc);

		return pkb;
	}
}
