#include <cassert>

#include <zpr.h>

#include "ast.h"
#include "pkb.h"
#include "util.h"

namespace pkb
{
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

    // Start of symbol table methods

    ast::VarRef* SymbolTable::get_var(std::string& str)
    {
        return _vars[str];
    }

    bool SymbolTable::has_var(std::string& str)
    {
        return _vars.count(str);
    }

    void SymbolTable::set_var(std::string& str, ast::VarRef* var)
    {
        _vars[str] = var;
    }

    ast::Procedure* SymbolTable::get_proc(std::string& str)
    {
        return _procs[str];
    }

    bool SymbolTable::has_proc(std::string& str)
    {
        return _procs.count(str);
    }

    void SymbolTable::set_proc(std::string& str, ast::Procedure* var)
    {
        _procs[str] = var;
    }

    void SymbolTable::populate(ast::Program* prog)
    {
        for(auto proc : prog->procedures)
        {
            set_proc(proc->name, proc);
            populate(proc->body);
        }
    }

    void SymbolTable::populate(ast::StmtList lst)
    {
        for(ast::Stmt* s : lst.statements)
        {
            if(ast::IfStmt* stmt = dynamic_cast<ast::IfStmt*>(s))
            {
                populate(stmt->condition);
                populate(stmt->true_case);
                populate(stmt->false_case);
            }
            else if(ast::WhileLoop* stmt = dynamic_cast<ast::WhileLoop*>(s))
            {
                populate(stmt->condition);
                populate(stmt->body);
            }
            else if(ast::AssignStmt* stmt = dynamic_cast<ast::AssignStmt*>(s))
            {
                populate(stmt->rhs);
            }
        }
    }

    void SymbolTable::populate(ast::Expr* e)
    {
        if(ast::VarRef* expr = dynamic_cast<ast::VarRef*>(e))
        {
            set_var(expr->name, expr);
        }
        else if(ast::BinaryOp* expr = dynamic_cast<ast::BinaryOp*>(e))
        {
            populate(expr->lhs);
            populate(expr->rhs);
        }
        else if(ast::UnaryOp* expr = dynamic_cast<ast::UnaryOp*>(e))
        {
            populate(expr->expr);
        }
    }

    // End of symbol table methods

    ProgramKB* processProgram(ast::Program* program)
    {
        auto pkb = new ProgramKB();

        // do a first pass to number all the statements, set the
        // parent stmtlist, and collect all the procedures.
        for(const auto& proc : program->procedures)
        {
            collectStmtList(pkb, &proc->body);

            if(pkb->procedures.find(proc->name) != pkb->procedures.end())
                util::error("pkb", "procedure '{}' is already defined", proc->name);

            pkb->procedures[proc->name].ast_proc = proc;
        }


        return pkb;
    }
}
