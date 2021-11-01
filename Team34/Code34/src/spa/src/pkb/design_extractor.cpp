// design_extractor.cpp

#include <queue>
#include <cassert>
#include <algorithm>
#include <functional>

#include <zpr.h>
#include <iostream>
#include <unordered_map>
#include <utility>
#include "pkb.h"
#include "util.h"
#include "timer.h"
#include "simple/ast.h"
#include "exceptions.h"
#include "pql/parser/ast.h"
#include "design_extractor.h"

namespace pkb
{
    namespace s_ast = simple::ast;
    using StatementNum = simple::ast::StatementNum;
    using DesignEnt = pql::ast::DESIGN_ENT;

#define CONST_DCAST(AstType, value) dynamic_cast<const s_ast::AstType*>(value)

    void DesignExtractor::assignStatementNumbersAndProc(const s_ast::StmtList* list, const s_ast::Procedure* proc)
    {
        std::function<void(s_ast::Stmt*, const s_ast::StmtList*)> processor {};
        processor = [this, &processor, proc](s_ast::Stmt* stmt, const s_ast::StmtList* parent) -> void {
            // the statement should not have been seen yet.
            assert(stmt->id == 0);

            stmt->parent_list = parent;
            stmt->id = m_pkb->m_statements.size() + 1;
            m_pkb->m_statements.emplace_back(stmt);
            m_pkb->getStatementAt(stmt->id).proc = proc;

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

        var.m_used_by_procs.insert(ts.current_proc->getName());
        ts.current_proc->m_uses.insert(varname);

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

        var.m_modified_by_procs.insert(ts.current_proc->getName());
        ts.current_proc->m_modifies.insert(varname);
    }

    void DesignExtractor::processStmtList(const s_ast::StmtList* list, TraversalState& ts)
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

            m_pkb->m_stmt_kinds[DesignEnt::STMT].insert(sid);
            m_pkb->m_stmt_kinds[DesignEnt::PROG_LINE].insert(sid);

            if(auto if_stmt = CONST_DCAST(IfStmt, ast_stmt); if_stmt)
            {
                ts.local_stmt_stack.push_back(stmt);

                this->processExpr(if_stmt->condition.get(), stmt, ts);
                this->processStmtList(&if_stmt->true_case, ts);
                this->processStmtList(&if_stmt->false_case, ts);

                m_pkb->m_stmt_kinds[DesignEnt::IF].insert(sid);

                assert(ts.local_stmt_stack.back() == stmt);
                ts.local_stmt_stack.pop_back();
            }
            else if(auto while_loop = CONST_DCAST(WhileLoop, ast_stmt); while_loop)
            {
                ts.local_stmt_stack.push_back(stmt);

                this->processExpr(while_loop->condition.get(), stmt, ts);
                this->processStmtList(&while_loop->body, ts);

                m_pkb->m_stmt_kinds[DesignEnt::WHILE].insert(sid);

                assert(ts.local_stmt_stack.back() == stmt);
                ts.local_stmt_stack.pop_back();
            }
            else if(auto assign_stmt = CONST_DCAST(AssignStmt, ast_stmt); assign_stmt)
            {
                this->processModifies(assign_stmt->lhs, stmt, ts);
                this->processExpr(assign_stmt->rhs.get(), stmt, ts);

                m_pkb->m_stmt_kinds[DesignEnt::ASSIGN].insert(sid);
            }
            else if(auto read_stmt = CONST_DCAST(ReadStmt, ast_stmt); read_stmt)
            {
                this->processModifies(read_stmt->var_name, stmt, ts);
                m_pkb->getVariableNamed(read_stmt->var_name).m_read_stmts.insert(sid);

                m_pkb->m_stmt_kinds[DesignEnt::READ].insert(sid);
            }
            else if(auto print_stmt = CONST_DCAST(PrintStmt, ast_stmt); print_stmt)
            {
                this->processUses(print_stmt->var_name, stmt, ts);
                m_pkb->getVariableNamed(print_stmt->var_name).m_print_stmts.insert(sid);

                m_pkb->m_stmt_kinds[DesignEnt::PRINT].insert(sid);
            }
            else if(auto call_stmt = CONST_DCAST(ProcCall, ast_stmt); call_stmt)
            {
                // check for (a) nonexistent procedures
                if(m_pkb->m_procedures.find(call_stmt->proc_name) == m_pkb->m_procedures.end())
                    throw util::PkbException("pkb", "call to undefined procedure '{}'", call_stmt->proc_name);

                auto& callee = m_pkb->getProcedureNamed(call_stmt->proc_name);
                callee.m_call_stmts.insert(sid);

                m_pkb->m_stmt_kinds[DesignEnt::CALL].insert(sid);

                // assert(m_visited_procs.find(call_stmt->proc_name) != m_visited_procs.end());
                auto target = &m_pkb->getProcedureNamed(call_stmt->proc_name);
                for(auto used : target->getUsedVariables())
                    processUses(used, stmt, ts);

                for(auto modified : target->getModifiedVariables())
                    processModifies(modified, stmt, ts);
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
        auto cfg = this->m_pkb->m_cfg.get();

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

                if(auto assign_stmt = CONST_DCAST(AssignStmt, ast_stmt); assign_stmt)
                {
                    cfg->addAssignStmtMapping(sid, stmt);
                    cfg->addModStmtMapping(sid, stmt);
                }
                else if(auto read_stmt = CONST_DCAST(ReadStmt, ast_stmt); read_stmt)
                    cfg->addModStmtMapping(sid, stmt);
                else if(auto proc_call = CONST_DCAST(ProcCall, ast_stmt); proc_call)
                {
                    cfg->addModStmtMapping(sid, stmt);
                    cfg->addCallStmtMapping(sid, stmt);
                }
            }
        }
    }

    void DesignExtractor::processNextRelations()
    {
        // get adj of of direct nexts first
        m_pkb->m_cfg = std::make_unique<CFG>(m_pkb->m_statements.size());
        for(auto& [name, proc] : m_pkb->m_procedures)
        {
            auto body = &proc.getAstProc()->body;
            this->processCFG(body, 0);
        }
        processBipRelations();
        // get all shortest paths
        this->m_pkb->m_cfg->computeDistMat();
    }

    void DesignExtractor::processBipRelations()
    {
        auto cfg = this->m_pkb->m_cfg.get();
        auto adjMat = cfg->adj_mat;
        auto adjMatBip = cfg->adj_mat_bip;
        for(int i = 0; i < cfg->total_inst; i++)
        {
            for(int j = 0; j < cfg->total_inst; j++)
            {
                cfg->addEdgeBip(i+1, j+1, adjMat[i][j]);
            }
        }
        // get the last stmt(s)
        auto getLastStmts = [](const s_ast::StmtList* stmtLst) {
            std::vector<StatementNum> lastStmts {};
            std::function<void(const s_ast::StmtList*)> visitStmtList {};
            visitStmtList = [&](const s_ast::StmtList* stmtLst) {
                auto lastStmt = stmtLst->statements.back().get();
                if(auto stmt = CONST_DCAST(IfStmt, lastStmt); stmt)
                {
                    visitStmtList(&stmt->true_case);
                    visitStmtList(&stmt->false_case);
                }
                else
                {
                    lastStmts.push_back(stmtLst->statements.back().get()->id);
                }
            };
            visitStmtList(stmtLst);
            return lastStmts;
        };

        for(auto& [name, proc] : m_pkb->m_procedures)
        {
            auto stmtList = &proc.getAstProc()->body;
            auto pair = std::make_pair(stmtList->statements.begin()->get()->id, getLastStmts(&proc.getAstProc()->body));
            cfg->gates.insert({ name, pair });
        }
        for(auto& [name, proc] : m_pkb->m_procedures)
        {
            for(auto& callStmt : proc.getCallStmts())
            {
                auto nextStmt = cfg->getNextStatements(callStmt);
                assert(nextStmt.size() <= 1);
                if(nextStmt.size() != 0)
                {
                    cfg->addEdgeBip(callStmt, *nextStmt.begin(), SIZE_MAX);
                }
                auto calledProc = CONST_DCAST(ProcCall, this->m_pkb->getStatementAt(callStmt).getAstStmt())->proc_name;
                cfg->addEdgeBip(callStmt, cfg->gates.at(calledProc).first, callStmt + 1);
                if(nextStmt.size() != 0)
                {
                    for(auto from : cfg->gates.at(calledProc).second)
                    {
                        cfg->addEdgeBip(from, *nextStmt.begin(), callStmt + 1);
                    }
                }
            }
        }
    }




    std::vector<Procedure*> DesignExtractor::processCallGraph()
    {
        std::vector<pkb::Procedure*> unseen {};
        std::unordered_map<pkb::Procedure*, int> marks {};

        std::vector<pkb::Procedure*> topo_order {};

        for(auto& [n, p] : m_pkb->m_procedures)
            unseen.push_back(&p);

        std::function<void(pkb::Procedure*)> visit {};
        visit = [&](pkb::Procedure* p) {
            if(marks[p] == 2)
                return;

            else if(marks[p] == 1)
                throw util::PkbException("pkb", "illegal cyclic/recursive call");

            marks[p] = 1;

            std::function<void(const s_ast::StmtList*, pkb::Procedure*)> visit_stmt {};
            visit_stmt = [&](const s_ast::StmtList* list, pkb::Procedure* proc) {
                for(auto& _stmt : list->statements)
                {
                    const auto* stmt = _stmt.get();
                    if(auto i = CONST_DCAST(IfStmt, stmt); i)
                    {
                        visit_stmt(&i->true_case, p);
                        visit_stmt(&i->false_case, p);
                    }
                    else if(auto w = CONST_DCAST(WhileLoop, stmt); w)
                    {
                        visit_stmt(&w->body, p);
                    }
                    else if(auto c = CONST_DCAST(ProcCall, stmt); c)
                    {
                        m_pkb->m_calls_exists = true;

                        auto target = &m_pkb->getProcedureNamed(c->proc_name);
                        visit(target);

                        proc->m_calls.insert(c->proc_name);
                        target->m_called_by.insert(proc->getName());

                        // we can do calls_transitive properly here, since we are going *deeper*.
                        proc->m_calls_transitive.insert(c->proc_name);
                        proc->m_calls_transitive.insert(
                            target->m_calls_transitive.begin(), target->m_calls_transitive.end());

                        // called_by_transitive will be done outside, once we have established the
                        // topological order.
                    }
                }
            };

            visit_stmt(&p->getAstProc()->body, p);
            topo_order.push_back(p);
            marks[p] = 2;
        };

        while(unseen.size() > 0)
        {
            auto p = unseen.back();
            unseen.pop_back();
            visit(p);
        }

        // now that the order is established, populate called_by_transitive by iterating in reverse.
        for(auto it = topo_order.rbegin(); it != topo_order.rend(); ++it)
        {
            auto proc = *it;
            for(const auto& _caller : proc->m_called_by)
            {
                auto* caller = &m_pkb->getProcedureNamed(_caller);
                proc->m_called_by_transitive.insert(_caller);
                proc->m_called_by_transitive.insert(
                    caller->m_called_by_transitive.begin(), caller->m_called_by_transitive.end());
            }
        }

        return topo_order;
    }


    std::unique_ptr<ProgramKB> DesignExtractor::run()
    {
        START_BENCHMARK_TIMER("design extractor");
        // assign the statement numbers. this has to use the vector of procedures in
        // m_program, since the numbering depends on the order.
        for(const auto& proc : m_program->procedures)
        {
            m_pkb->addProcedure(proc->name, proc.get());
            this->assignStatementNumbersAndProc(&proc->body, proc.get());
        }

        auto topo_order = this->processCallGraph();
        for(auto* proc : topo_order)
        {
            auto body = &proc->getAstProc()->body;

            TraversalState ts {};
            ts.current_proc = proc;

            this->processStmtList(body, ts);
            m_visited_procs.insert(proc->getName());
        }

        this->processNextRelations();
        return std::move(this->m_pkb);
    }

        std::unique_ptr<ProgramKB> DesignExtractor::run2()
    {
        START_BENCHMARK_TIMER("design extractor");
        // assign the statement numbers. this has to use the vector of procedures in
        // m_program, since the numbering depends on the order.
        for(const auto& proc : m_program->procedures)
        {
            m_pkb->addProcedure(proc->name, proc.get());
            this->assignStatementNumbersAndProc(&proc->body, proc.get());
        }

        auto topo_order = this->processCallGraph();
        for(auto* proc : topo_order)
        {
            auto body = &proc->getAstProc()->body;

            TraversalState ts {};
            ts.current_proc = proc;

            this->processStmtList(body, ts);
            m_visited_procs.insert(proc->getName());
        }

        this->processNextRelations();
        this->m_pkb->m_cfg->computeDistMatBip();

        return std::move(this->m_pkb);
    }


    DesignExtractor::DesignExtractor(std::unique_ptr<s_ast::Program> program)
    {
        this->m_pkb = std::make_unique<ProgramKB>(std::move(program));
        this->m_program = this->m_pkb->getProgram();

        // pre-populate the kinds in the pkb with all the valid stmt thingies
        for(auto stmt_ent : pql::ast::getStmtDesignEntities())
            m_pkb->m_stmt_kinds[stmt_ent] = {};
    }
}
