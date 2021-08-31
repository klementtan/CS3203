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

    ast::VarRef* SymbolTable::getVar(const std::string& str)
    {
        if(!hasVar(str))
            throw "Key does not exist!";
        return _vars[str];
    }

    bool SymbolTable::hasVar(const std::string& str)
    {
        return _vars.count(str);
    }

    void SymbolTable::setVar(const std::string& str, ast::VarRef* var)
    {
        _vars[str] = var;
    }

    ast::Procedure* SymbolTable::getProc(const std::string& str)
    {
        if(!hasProc(str))
            throw "Key does not exist!";
        return _procs[str];
    }

    bool SymbolTable::hasProc(const std::string& str)
    {
        return _procs.count(str);
    }

    void SymbolTable::setProc(const std::string& str, ast::Procedure* var)
    {
        _procs[str] = var;
    }

    void SymbolTable::populate(ast::Program* prog)
    {
        for(auto proc : prog->procedures)
        {
            setProc(proc->name, proc);
            populate(proc->body);
        }
    }

    void SymbolTable::populate(const ast::StmtList& lst)
    {
        for(auto stmt : lst.statements)
        {
            if(ast::IfStmt* if_stmt = dynamic_cast<ast::IfStmt*>(stmt))
            {
                populate(if_stmt->condition);
                populate(if_stmt->true_case);
                populate(if_stmt->false_case);
            }
            else if(ast::WhileLoop* while_stmt = dynamic_cast<ast::WhileLoop*>(stmt))
            {
                populate(while_stmt->condition);
                populate(while_stmt->body);
            }
            else if(ast::AssignStmt* assign_stmt = dynamic_cast<ast::AssignStmt*>(stmt))
            {
                populate(assign_stmt->rhs);
            }
        }
    }

    void SymbolTable::populate(ast::Expr* expr)
    {
        if(ast::VarRef* var_ref = dynamic_cast<ast::VarRef*>(expr))
        {
            setVar(var_ref->name, var_ref);
        }
        else if(ast::BinaryOp* binary_op = dynamic_cast<ast::BinaryOp*>(expr))
        {
            populate(binary_op->lhs);
            populate(binary_op->rhs);
        }
        else if(ast::UnaryOp* unary_op = dynamic_cast<ast::UnaryOp*>(expr))
        {
            populate(unary_op->expr);
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
