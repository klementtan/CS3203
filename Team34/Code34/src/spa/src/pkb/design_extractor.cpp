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
    using StatementNum = simple::ast::StatementNum;

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
        var.m_used_by.insert(stmt);
        for(auto s : ts.local_stmt_stack)
        {
            var.m_used_by.insert(s);
            m_pkb->getStatementAt(s->getStmtNum()).m_uses.insert(varname);
        }

        for(auto s : ts.global_stmt_stack)
        {
            var.m_used_by.insert(s);
            m_pkb->getStatementAt(s->getStmtNum()).m_uses.insert(varname);
        }

        for(auto proc : ts.proc_stack)
        {
            var.m_used_by_procs.insert(proc->getName());
            m_pkb->getProcedureNamed(proc->getName()).m_uses.insert(varname);
        }

        for(auto call : ts.call_stack)
        {
            call->m_uses.insert(varname);
            var.m_used_by.insert(call);
        }

        // populate condition_uses for ifs and whiles
        if(auto astmt = stmt->getAstStmt(); CONST_DCAST(WhileLoop, astmt) || CONST_DCAST(IfStmt, astmt))
            stmt->m_condition_uses.insert(varname);
    }


    void DesignExtractor::processModifies(const std::string& varname, Statement* stmt, const TraversalState& ts)
    {
        stmt->m_modifies.insert(varname);

        auto& var = m_pkb->m_variables[varname];

        // transitively add the modifies
        var.m_modified_by.insert(stmt);
        for(auto s : ts.local_stmt_stack)
        {
            var.m_modified_by.insert(s);
            m_pkb->getStatementAt(s->getStmtNum()).m_modifies.insert(varname);
        }

        for(auto s : ts.global_stmt_stack)
        {
            var.m_modified_by.insert(s);
            m_pkb->getStatementAt(s->getStmtNum()).m_modifies.insert(varname);
        }

        for(auto proc : ts.proc_stack)
        {
            var.m_modified_by_procs.insert(proc->getName());
            m_pkb->getProcedureNamed(proc->getName()).m_modifies.insert(varname);
        }

        for(auto call : ts.call_stack)
        {
            call->m_modifies.insert(varname);
            var.m_modified_by.insert(call);
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
            auto this_stmt = &m_pkb->getStatementAt(this_id);

            if(i > 0)
            {
                auto prev_id = list->statements[i - 1]->id;
                auto prev_stmt = &m_pkb->getStatementAt(prev_id);

                this_stmt->m_directly_before = prev_id;
                this_stmt->m_before.insert(prev_id);
                this_stmt->m_before.insert(prev_stmt->m_before.begin(), prev_stmt->m_before.end());

                m_pkb->m_follows_exists = true;
            }
        }

        for(size_t i = list->statements.size(); i-- > 0;)
        {
            auto this_id = list->statements[i]->id;
            auto this_stmt = &m_pkb->getStatementAt(this_id);

            if(i > 0)
            {
                auto prev_id = list->statements[i - 1]->id;
                auto prev_stmt = &m_pkb->getStatementAt(prev_id);

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
            auto stmt = &m_pkb->getStatementAt(ast_stmt->id);
            auto sid = ast_stmt->id;

            // we really only need to look at the last thing in the stack.
            if(ts.local_stmt_stack.size() > 0)
            {
                auto list = ts.local_stmt_stack.back();
                auto list_sid = list->getStmtNum();
                stmt->m_parent = { list_sid };

                list->m_children.insert(sid);

                // m_pkb->m_direct_parents[sid] = list_sid;
                // m_pkb->m_direct_children[list_sid].insert(sid);

                // the ancestors of this statement are simply:
                // 1. the direct parent (list_sid)
                // 2. the ancestors of the parent (list_sid->ancestors)
                stmt->m_ancestors.insert(list_sid);
                stmt->m_ancestors.insert(list->m_ancestors.begin(), list->m_ancestors.end());

                // for populating descendants, we still need to traverse upwards.
                for(auto slist : ts.local_stmt_stack)
                    slist->m_descendants.insert(sid);

                m_pkb->m_parent_exists = true;
            }



            if(auto if_stmt = CONST_DCAST(IfStmt, ast_stmt); if_stmt)
            {
                auto new_ts = ts;
                new_ts.local_stmt_stack.push_back(stmt);
                new_ts.global_stmt_stack.push_back(stmt);

                this->processExpr(if_stmt->condition.get(), stmt, new_ts);
                this->processStmtList(&if_stmt->true_case, new_ts);
                this->processStmtList(&if_stmt->false_case, new_ts);
            }
            else if(auto while_loop = CONST_DCAST(WhileLoop, ast_stmt); while_loop)
            {
                auto new_ts = ts;
                new_ts.local_stmt_stack.push_back(stmt);
                new_ts.global_stmt_stack.push_back(stmt);

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
                m_pkb->getVariableNamed(read_stmt->var_name).m_read_stmts.insert(sid);
            }
            else if(auto print_stmt = CONST_DCAST(PrintStmt, ast_stmt); print_stmt)
            {
                this->processUses(print_stmt->var_name, stmt, ts);
                m_pkb->getVariableNamed(print_stmt->var_name).m_print_stmts.insert(sid);
            }
            else if(auto call_stmt = CONST_DCAST(ProcCall, ast_stmt); call_stmt)
            {
                // check for (a) nonexistent procedures
                if(m_pkb->m_procedures.find(call_stmt->proc_name) == m_pkb->m_procedures.end())
                    throw util::PkbException("pkb", "call to undefined procedure '{}'", call_stmt->proc_name);

                auto& callee = m_pkb->getProcedureNamed(call_stmt->proc_name);
                auto* body = &callee.getAstProc()->body;

                callee.m_call_stmts.insert(sid);

                // (b) cyclic calls
                if(std::find(ts.proc_stack.begin(), ts.proc_stack.end(), &callee) != ts.proc_stack.end())
                    throw util::PkbException("pkb", "illegal cyclic/recursive call", call_stmt->proc_name);

                m_pkb->m_calls_exists = true;
                for(size_t i = 0; i < ts.proc_stack.size(); i++)
                {
                    // only set the direct calls/called_by for the top of the stack
                    if(i == ts.proc_stack.size() - 1)
                    {
                        ts.proc_stack[i]->m_calls.insert(callee.getName());
                        callee.m_called_by.insert(ts.proc_stack[i]->getName());
                    }

                    ts.proc_stack[i]->m_calls_transitive.insert(callee.getName());
                    callee.m_called_by_transitive.insert(ts.proc_stack[i]->getName());
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

    void DesignExtractor::processCFG(const s_ast::StmtList* list, StatementNum last_checkpt)
    {
        auto cfg = this->m_pkb->cfg.get();

        // there must be a 'flow' from parent to first stmt of its body
        if(list->parent_statement != nullptr)
        {
            if(StatementNum parent_id = list->parent_statement->id; parent_id != 0)
            {
                cfg->addEdge(parent_id, list->statements[0].get()->id);
            }
        }

        for(const auto& it : list->statements)
        {
            const auto ast_stmt = it.get();
            auto stmt = &m_pkb->getStatementAt(ast_stmt->id);
            auto sid = ast_stmt->id;
            StatementNum nextStmtId = stmt->getStmtDirectlyAfter();

            if(auto if_stmt = CONST_DCAST(IfStmt, ast_stmt); if_stmt)
            {
                this->processCFG(&if_stmt->true_case,
                    nextStmtId == 0 ? last_checkpt : nextStmtId); // If 'if' is at the end of stmtlist, loop back
                this->processCFG(&if_stmt->false_case, nextStmtId == 0 ? last_checkpt : nextStmtId);
            }
            else
            {
                if(nextStmtId != 0)
                    cfg->addEdge(sid, nextStmtId); // not the end of stmtlist so we don't need to loop back yet
                else if(last_checkpt != 0)
                    cfg->addEdge(sid, last_checkpt); // only non-if stmts can loop back
                if(auto while_loop = CONST_DCAST(WhileLoop, ast_stmt); while_loop)
                    this->processCFG(&while_loop->body, sid);
            }
        }
    }

    void DesignExtractor::processNextRelations()
    {
        // get adj of of direct nexts first
        m_pkb->cfg = std::make_unique<CFG>(m_pkb->m_statements.size());
        for(auto& [name, proc] : m_pkb->m_procedures)
        {
            auto body = &proc.getAstProc()->body;
            this->processCFG(body, 0);
        }
        // get all shortest paths
        auto cfg = this->m_pkb->cfg.get();
        cfg->computeDistMat();
    }

    std::unique_ptr<ProgramKB> DesignExtractor::run()
    {
        // assign the statement numbers. this has to use the vector of procedures in
        // m_program, since the numbering depends on the order.
        for(const auto& proc : m_program->procedures)
        {
            m_pkb->addProcedure(proc->name, proc.get());
            this->assignStatementNumbers(&proc->body);
        }

        // process the entire thing
        for(auto& [name, proc] : m_pkb->m_procedures)
        {
            if(m_visited_procs.find(name) != m_visited_procs.end())
                continue;

            auto body = &proc.getAstProc()->body;

            TraversalState ts {};
            ts.proc_stack.push_back(&proc);

            this->processStmtList(body, ts);
        }

        processNextRelations();

        return std::move(this->m_pkb);
    }

    DesignExtractor::DesignExtractor(std::unique_ptr<s_ast::Program> program)
    {
        this->m_pkb = std::make_unique<ProgramKB>(std::move(program));
        this->m_program = this->m_pkb->getProgram();
    }
}
