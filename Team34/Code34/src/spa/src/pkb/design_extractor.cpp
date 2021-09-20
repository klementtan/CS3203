// design_extractor.cpp

#include <queue>
#include <cassert>
#include <algorithm>
#include <functional>

#include <zpr.h>

#include "pkb.h"
#include "util.h"
#include "simple/ast.h"
#include "exceptions.h"
#include "design_extractor.h"

namespace pkb
{
    namespace s_ast = simple::ast;

#define CONST_DCAST(AstType, value) dynamic_cast<const s_ast::AstType*>(value)

    void DesignExtractor::assignStatementNumbers(const s_ast::StmtList* list)
    {
        std::function<void(s_ast::Stmt*, const s_ast::StmtList*)> processor {};
        processor = [this, &processor](s_ast::Stmt* stmt, const s_ast::StmtList* parent) -> void {
            // the statement should not have been seen yet.
            assert(stmt->id == 0);

            stmt->parent_list = parent;
            stmt->id = m_pkb->m_statements.size() + 1;
            m_pkb->m_statements.emplace_back(stmt);

            if(auto i = dynamic_cast<s_ast::IfStmt*>(stmt); i)
            {
                for(const auto& stmt : i->true_case.statements)
                    processor(stmt.get(), &i->true_case);

                for(const auto& stmt : i->false_case.statements)
                    processor(stmt.get(), &i->false_case);

                i->true_case.parent_statement = stmt;
                i->false_case.parent_statement = stmt;
            }
            else if(auto w = dynamic_cast<s_ast::WhileLoop*>(stmt); w)
            {
                for(const auto& stmt : w->body.statements)
                    processor(stmt.get(), &w->body);

                w->body.parent_statement = stmt;
            }
        };


        for(const auto& stmt : list->statements)
            processor(stmt.get(), list);
    }

    void DesignExtractor::processUses(const std::string& varname, Statement* stmt, const TraversalState& ts)
    {
        stmt->m_uses.insert(varname);

        auto& var = m_pkb->m_variables[varname];

        // transitively add the modifies
        var.used_by.insert(stmt->getAstStmt());
        for(auto s : ts.local_stmt_stack)
        {
            var.used_by.insert(s);
            m_pkb->getStatementAtIndex(s->id)->m_uses.insert(varname);
        }

        for(auto s : ts.global_stmt_stack)
        {
            var.used_by.insert(s);
            m_pkb->getStatementAtIndex(s->id)->m_uses.insert(varname);
        }

        for(auto proc : ts.proc_stack)
        {
            var.used_by_procs.insert(proc);
            m_pkb->m_procedures[proc->ast_proc->name].uses.insert(varname);
        }

        for(auto call : ts.call_stack)
        {
            call->m_uses.insert(varname);
            var.used_by.insert(call->getAstStmt());
        }
    }


    void DesignExtractor::processModifies(const std::string& varname, Statement* stmt, const TraversalState& ts)
    {
        stmt->m_modifies.insert(varname);

        auto& var = m_pkb->m_variables[varname];

        // transitively add the modifies
        var.modified_by.insert(stmt->getAstStmt());
        for(auto s : ts.local_stmt_stack)
        {
            var.modified_by.insert(s);
            m_pkb->getStatementAtIndex(s->id)->m_modifies.insert(varname);
        }

        for(auto s : ts.global_stmt_stack)
        {
            var.modified_by.insert(s);
            m_pkb->getStatementAtIndex(s->id)->m_modifies.insert(varname);
        }

        for(auto proc : ts.proc_stack)
        {
            var.modified_by_procs.insert(proc);
            m_pkb->m_procedures[proc->ast_proc->name].modifies.insert(varname);
        }

        for(auto call : ts.call_stack)
        {
            call->m_modifies.insert(varname);
            var.modified_by.insert(call->getAstStmt());
        }
    }


    void DesignExtractor::processStmtList(const s_ast::StmtList* list, const TraversalState& ts)
    {
        // indices are easier to work with here.
        // do one pass forwards and one pass in reverse to effeciently set
        // follows and follows* in both directions.
        for(size_t i = 0; i < list->statements.size(); i++)
        {
            auto this_id = list->statements[i]->id;
            auto this_stmt = m_pkb->getStatementAtIndex(this_id);

            if(i > 0)
            {
                auto prev_id = list->statements[i - 1]->id;
                auto prev_stmt = m_pkb->getStatementAtIndex(prev_id);

                this_stmt->m_directly_before = prev_id;
                this_stmt->m_before.insert(prev_id);
                this_stmt->m_before.insert(prev_stmt->m_before.begin(), prev_stmt->m_before.end());

                m_pkb->m_follows_exists = true;
            }
        }

        for(size_t i = list->statements.size(); i-- > 0;)
        {
            auto this_id = list->statements[i]->id;
            auto this_stmt = m_pkb->getStatementAtIndex(this_id);

            if(i > 0)
            {
                auto prev_id = list->statements[i - 1]->id;
                auto prev_stmt = m_pkb->getStatementAtIndex(prev_id);

                prev_stmt->m_directly_after = this_id;
                prev_stmt->m_after.insert(this_id);
                prev_stmt->m_after.insert(this_stmt->m_after.begin(), this_stmt->m_after.end());

                m_pkb->m_follows_exists = true;
            }
        }



        // now process uses, modifies, parent, and parent* in one go
        for(const auto& it : list->statements)
        {
            const auto ast_stmt = it.get();

            // set the parent and children accordingly
            auto stmt = m_pkb->getStatementAtIndex(ast_stmt->id);
            auto sid = ast_stmt->id;

            for(size_t i = 0; i < ts.local_stmt_stack.size(); i++)
            {
                auto list_sid = ts.local_stmt_stack[i]->id;

                // only set the direct parent/child for the top of the stack
                if(i == ts.local_stmt_stack.size() - 1)
                {
                    m_pkb->m_direct_parents[sid] = list_sid;
                    m_pkb->m_direct_children[list_sid].insert(sid);
                }

                m_pkb->m_ancestors[sid].insert(list_sid);
                m_pkb->m_descendants[list_sid].insert(sid);

                m_pkb->m_parent_exists = true;
            }


            if(auto if_stmt = CONST_DCAST(IfStmt, ast_stmt); if_stmt)
            {
                auto new_ts = ts;
                new_ts.local_stmt_stack.push_back(if_stmt);
                new_ts.global_stmt_stack.push_back(if_stmt);

                this->processExpr(if_stmt->condition.get(), stmt, new_ts);
                this->processStmtList(&if_stmt->true_case, new_ts);
                this->processStmtList(&if_stmt->false_case, new_ts);
            }
            else if(auto while_loop = CONST_DCAST(WhileLoop, ast_stmt); while_loop)
            {
                auto new_ts = ts;
                new_ts.local_stmt_stack.push_back(while_loop);
                new_ts.global_stmt_stack.push_back(while_loop);

                this->processExpr(while_loop->condition.get(), stmt, new_ts);
                this->processStmtList(&while_loop->body, new_ts);
            }
            else if(auto assign_stmt = CONST_DCAST(AssignStmt, ast_stmt); assign_stmt)
            {
                this->processModifies(assign_stmt->lhs, stmt, ts);
                this->processExpr(assign_stmt->rhs.get(), stmt, ts);
            }
            else if(auto read_stmt = CONST_DCAST(ReadStmt, ast_stmt); read_stmt)
            {
                this->processModifies(read_stmt->var_name, stmt, ts);
            }
            else if(auto print_stmt = CONST_DCAST(PrintStmt, ast_stmt); print_stmt)
            {
                this->processUses(print_stmt->var_name, stmt, ts);
            }
            else if(auto call_stmt = CONST_DCAST(ProcCall, ast_stmt); call_stmt)
            {
                // check for (a) nonexistent procedures
                if(m_pkb->m_procedures.find(call_stmt->proc_name) == m_pkb->m_procedures.end())
                    throw util::PkbException("pkb", "call to undefined procedure '{}'", call_stmt->proc_name);

                auto& callee = m_pkb->getProcedureNamed(call_stmt->proc_name);
                auto* body = &callee.ast_proc->body;

                // (b) cyclic calls
                if(std::find(ts.proc_stack.begin(), ts.proc_stack.end(), &callee) != ts.proc_stack.end())
                    throw util::PkbException("pkb", "illegal cyclic/recursive call", call_stmt->proc_name);


                for(size_t i = 0; i < ts.proc_stack.size(); i++)
                {
                    // only set the direct calls/called_by for the top of the stack
                    if(i == ts.proc_stack.size() - 1)
                    {
                        ts.proc_stack[i]->calls.insert(callee.ast_proc->name);
                        callee.called_by.insert(ts.proc_stack[i]->ast_proc->name);
                    }

                    ts.proc_stack[i]->calls_transitive.insert(callee.ast_proc->name);
                    callee.called_by_transitive.insert(ts.proc_stack[i]->ast_proc->name);
                }

                auto new_ts = ts;
                new_ts.local_stmt_stack.clear();
                new_ts.proc_stack.push_back(&callee);
                new_ts.call_stack.push_back(stmt);

                m_visited_procs.insert(call_stmt->proc_name);

                this->processStmtList(body, new_ts);
            }
            else
            {
                throw util::PkbException("pkb", "invalid statement type");
            }
        }
    }

    void DesignExtractor::processExpr(const s_ast::Expr* expr, Statement* stmt, const TraversalState& ts)
    {
        if(auto vr = CONST_DCAST(VarRef, expr); vr)
        {
            this->processUses(vr->name, stmt, ts);
        }
        else if(auto cnst = CONST_DCAST(Constant, expr); cnst)
        {
            m_pkb->addConstant(cnst->value);
        }
        else if(auto binop = CONST_DCAST(BinaryOp, expr); binop)
        {
            this->processExpr(binop->lhs.get(), stmt, ts);
            this->processExpr(binop->rhs.get(), stmt, ts);
        }
        else if(auto unaryop = CONST_DCAST(UnaryOp, expr); unaryop)
        {
            this->processExpr(unaryop->expr.get(), stmt, ts);
        }
        else
        {
            throw util::PkbException("pkb", "unknown expression type");
        }
    }

    std::unique_ptr<ProgramKB> DesignExtractor::run()
    {
        // assign the statement numbers. this has to use the vector of procedures in
        // m_program, since the numbering depends on the order.
        for(const auto& proc : m_program->procedures)
        {
            // collectStmtList(pkb.get(), &proc->body);
            this->assignStatementNumbers(&proc->body);
            m_pkb->m_procedures[proc->name].ast_proc = proc.get();
        }

        // process the entire thing
        for(auto& [name, proc] : m_pkb->m_procedures)
        {
            if(m_visited_procs.find(name) != m_visited_procs.end())
                continue;

            auto body = &proc.ast_proc->body;

            TraversalState ts {};
            ts.proc_stack.push_back(&proc);

            this->processStmtList(body, ts);
        }

        return std::move(this->m_pkb);
    }

    DesignExtractor::DesignExtractor(std::unique_ptr<s_ast::Program> program)
    {
        this->m_pkb = std::make_unique<ProgramKB>(std::move(program));
        this->m_program = this->m_pkb->getProgram();
    }
}
