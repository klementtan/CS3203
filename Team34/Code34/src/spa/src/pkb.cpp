// pkb.cpp

#include <queue>
#include <cassert>
#include <algorithm>

#include <zpr.h>
#include <zst.h>

#include "pkb.h"
#include "util.h"
#include "simple/ast.h"
#include "exceptions.h"

namespace pkb
{
    using zst::Ok;
    using zst::Err;
    using zst::ErrFmt;

    template <typename T>
    using Result = zst::Result<T, std::string>;
    namespace s_ast = simple::ast;

    using zst::Ok;
    using zst::ErrFmt;

    // collection only entails numbering the statements
    static void collectStmtList(ProgramKB* pkb, s_ast::StmtList* list);
    static void collectStmt(ProgramKB* pkb, s_ast::Stmt* stmt, s_ast::StmtList* parent)
    {
        // the statement should not have been seen yet.
        assert(stmt->id == 0);

        stmt->parent_list = parent;
        stmt->id = pkb->uses_modifies.statements.size() + 1;
        auto statement = new Statement();
        statement->stmt = stmt;
        pkb->uses_modifies.statements.push_back(statement);

        if(auto i = dynamic_cast<s_ast::IfStmt*>(stmt); i)
        {
            collectStmtList(pkb, &i->true_case);
            collectStmtList(pkb, &i->false_case);
            i->true_case.parent_statement = stmt;
            i->false_case.parent_statement = stmt;
        }
        else if(auto w = dynamic_cast<s_ast::WhileLoop*>(stmt); w)
        {
            collectStmtList(pkb, &w->body);
            w->body.parent_statement = stmt;
        }
    }

    static void collectStmtList(ProgramKB* pkb, s_ast::StmtList* list)
    {
        for(const auto& stmt : list->statements)
            collectStmt(pkb, stmt, list);
    }

    // Start of symbol table methods

    s_ast::VarRef* SymbolTable::getVar(const std::string& str)
    {
        if(!hasVar(str))
            throw util::PkbException("pkb", "variable '{}' does not exist!", str);

        return _vars[str];
    }

    bool SymbolTable::hasVar(const std::string& str)
    {
        return _vars.count(str);
    }

    void SymbolTable::setVar(const std::string& str, s_ast::VarRef* var)
    {
        _vars[str] = var;
    }

    s_ast::Procedure* SymbolTable::getProc(const std::string& str)
    {
        if(!hasProc(str))
            throw util::PkbException("pkb", "procedure '{}' does not exist!", str);

        return _procs[str];
    }

    bool SymbolTable::hasProc(const std::string& str)
    {
        return _procs.count(str);
    }

    void SymbolTable::setProc(const std::string& str, s_ast::Procedure* var)
    {
        _procs[str] = var;
    }

    void SymbolTable::populate(s_ast::Program* prog)
    {
        for(auto proc : prog->procedures)
        {
            setProc(proc->name, proc);
            populate(proc->body);
        }
    }

    void SymbolTable::populate(const s_ast::StmtList& lst)
    {
        for(auto stmt : lst.statements)
        {
            if(s_ast::IfStmt* if_stmt = dynamic_cast<s_ast::IfStmt*>(stmt))
            {
                populate(if_stmt->condition);
                populate(if_stmt->true_case);
                populate(if_stmt->false_case);
            }
            else if(s_ast::WhileLoop* while_stmt = dynamic_cast<s_ast::WhileLoop*>(stmt))
            {
                populate(while_stmt->condition);
                populate(while_stmt->body);
            }
            else if(s_ast::AssignStmt* assign_stmt = dynamic_cast<s_ast::AssignStmt*>(stmt))
            {
                populate(assign_stmt->rhs);
            }
        }
    }

    void SymbolTable::populate(s_ast::Expr* expr)
    {
        if(s_ast::VarRef* var_ref = dynamic_cast<s_ast::VarRef*>(expr))
        {
            setVar(var_ref->name, var_ref);
        }
        else if(s_ast::BinaryOp* binary_op = dynamic_cast<s_ast::BinaryOp*>(expr))
        {
            populate(binary_op->lhs);
            populate(binary_op->rhs);
        }
        else if(s_ast::UnaryOp* unary_op = dynamic_cast<s_ast::UnaryOp*>(expr))
        {
            populate(unary_op->expr);
        }
    }

    // End of symbol table methods

    // processes and populates Follows and FollowsT concurrently
    static void processFollows(ProgramKB* pkb, s_ast::StmtList* list)
    {
        const auto& stmt_list = list->statements;
        for(size_t i = 0; i < stmt_list.size(); i++)
        {
            s_ast::Stmt* stmt = stmt_list[i];

            auto follows = new Follows();
            follows->id = stmt->id;
            pkb->follows.push_back(follows);

            for(size_t j = i + 1; j < stmt_list.size(); j++)
            {
                if(j - i == 1)
                {
                    follows->directly_after = stmt_list[j]->id;
                }
                follows->after.insert(stmt_list[j]->id);
                pkb->m_follows_exists = true;
            }

            for(size_t k = 0; k < i; k++)
            {
                if(i - k == 1)
                {
                    follows->directly_before = stmt_list[k]->id;
                }
                follows->before.insert(stmt_list[k]->id);
                pkb->m_follows_exists = true;
            }

            if(auto i = dynamic_cast<s_ast::IfStmt*>(stmt); i)
            {
                processFollows(pkb, &i->true_case);
                processFollows(pkb, &i->false_case);
            }
            else if(auto w = dynamic_cast<s_ast::WhileLoop*>(stmt); w)
            {
                processFollows(pkb, &w->body);
            }
        }
    }

    static void processCallGraph(ProgramKB* pkb, s_ast::StmtList* list, std::string* procName)
    {
        const auto& stmt_list = list->statements;
        for(s_ast::Stmt* stmt : stmt_list)
        {
            if(auto i = dynamic_cast<s_ast::ProcCall*>(stmt); i)
            {
                pkb->proc_calls.addEdge(*procName, i->proc_name);
            }
            else if(auto i = dynamic_cast<s_ast::IfStmt*>(stmt); i)
            {
                processCallGraph(pkb, &i->true_case, procName);
                processCallGraph(pkb, &i->false_case, procName);
            }
            else if(auto w = dynamic_cast<s_ast::WhileLoop*>(stmt); w)
            {
                processCallGraph(pkb, &w->body, procName);
            }
        }
    }

    void CallGraph::addEdge(std::string& a, std::string b)
    {
        adj[a].insert(std::move(b));
    }

    // runs dfs to detect cycle
    bool CallGraph::dfs(std::string a, std::unordered_map<std::string, std::unordered_set<std::string>>* tempAdj,
        std::unordered_set<std::string>* visited)
    {
        if(visited->find(a) != visited->end())
        {
            return true;
        }
        visited->insert(a);
        if(tempAdj->find(a) != tempAdj->end())
        {
            auto s = (*tempAdj)[a].begin();
            while(s != (*tempAdj)[a].end())
            {
                if(dfs(*s, tempAdj, visited))
                {
                    return true;
                }
                else
                {
                    s = (*tempAdj)[a].erase((*tempAdj)[a].find(*s));
                    if((*tempAdj)[a].empty())
                    {
                        tempAdj->erase(a);
                        break;
                    }
                }
            }
        }
        visited->erase(a);
        return false;
    }

    // runs dfs on each graph
    bool CallGraph::cycleExists()
    {
        std::unordered_map<std::string, std::unordered_set<std::string>> tempAdj;
        std::unordered_set<std::string> visited;
        for(auto i = adj.begin(); i != adj.end(); i++)
        {
            for(auto a : i->second)
            {
                tempAdj[i->first].insert(a.c_str());
            }
        }
        while(tempAdj.size() != 0)
        {
            bool res = dfs(tempAdj.begin()->first, &tempAdj, &visited);
            if(res)
                return true;
        }
        return false;
    }

    std::string CallGraph::missingProc(std::vector<s_ast::Procedure*> procs)
    {
        for(auto& a : adj)
        {
            for(auto& callee : a.second)
            {
                if(std::find_if(procs.begin(), procs.end(), [&callee](const auto& p) { return p->name == callee; }) ==
                    procs.end())
                {
                    return callee;
                }
            }
        }
        return "";
    }

    // Takes in two 1-indexed StatementNums
    bool ProgramKB::isFollows(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        // thinking of more elegant ways of handling this hmm
        if(fst > this->follows.size() || snd > this->follows.size() || fst < 1 || snd < 1)
            throw util::PkbException("pkb::eval", "StatementNum out of range.");

        return this->follows[fst - 1]->directly_after == snd;
    }

    // Same as isFollows
    bool ProgramKB::isFollowsT(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(fst > this->follows.size() || snd > this->follows.size() || fst < 1 || snd < 1)
            throw util::PkbException("pkb::eval", "StatementNum out of range.");

        return this->follows[fst - 1]->after.count(snd) > 0;
    }
    Follows* ProgramKB::getFollows(simple::ast::StatementNum fst)
    {
        if(fst > this->follows.size() || fst < 1)
            throw util::PkbException("pkb::eval", "StatementNum out of range.");
        return this->follows[fst - 1];
    }

    // Takes in two 1-indexed StatementNums. Allows for 0 to be used as a wildcard on 1 of the parameters.
    std::unordered_set<s_ast::StatementNum> ProgramKB::getFollowsTList(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        // note: no need to check for < 0 since StatementNum is unsigned
        if(fst > this->follows.size() || snd > this->follows.size())
            throw util::PkbException("pkb::eval", "StatementNum out of range.");

        if((fst < 1 && snd < 1) || (fst != 0 && snd != 0))
            throw util::PkbException("pkb::eval", "Only 1 wildcard is to be used.");

        if(fst == 0)
        {
            return this->follows[snd - 1]->before;
        }
        else if(snd == 0)
        {
            return this->follows[fst - 1]->after;
        }

        throw util::PkbException("pkb", "unreachable code reached");
    }

    /**
     * Start of Parent methods
     */

    // processes the parents and ancestors of statements
    static void processAncestors(ProgramKB* pkb, s_ast::StmtList* lst)
    {
        std::queue<std::pair<s_ast::Stmt*, s_ast::Stmt*>> q;

        for(auto stmt : lst->statements)
            q.push({ stmt, nullptr });

        while(!q.empty())
        {
            auto [child, parent] = q.front();
            q.pop();

            if(child == nullptr)
                continue;

            if(pkb->_ancestors.count(child->id) == 0)
            {
                std::unordered_set<s_ast::StatementNum> anc;
                pkb->_ancestors[child->id] = anc;
            }

            std::unordered_set<s_ast::StatementNum> anc = pkb->_ancestors[child->id];

            if(parent != nullptr)
            {
                pkb->m_parent_exists = true;

                anc.insert(parent->id);
                pkb->_direct_parents[child->id] = parent->id;
                for(auto num : pkb->_ancestors[parent->id])
                    anc.insert(num);
            }

            if(s_ast::IfStmt* if_stmt = dynamic_cast<s_ast::IfStmt*>(child))
            {
                for(auto inner : if_stmt->true_case.statements)
                    q.push({ inner, child });

                for(auto inner : if_stmt->false_case.statements)
                    q.push({ inner, child });
            }
            else if(s_ast::WhileLoop* while_stmt = dynamic_cast<s_ast::WhileLoop*>(child))
            {
                for(auto inner : while_stmt->body.statements)
                    q.push({ inner, child });
            }

            pkb->_ancestors[child->id] = anc;
        }
    }

    // processes the children and descendants of statements
    static void processDescendants(ProgramKB* pkb, s_ast::StmtList* lst)
    {
        for(auto stmt : lst->statements)
        {
            std::unordered_set<s_ast::StatementNum> chi;
            std::unordered_set<s_ast::StatementNum> des;

            if(s_ast::IfStmt* if_stmt = dynamic_cast<s_ast::IfStmt*>(stmt))
            {
                processDescendants(pkb, &if_stmt->true_case);
                processDescendants(pkb, &if_stmt->false_case);

                for(auto inner : if_stmt->true_case.statements)
                {
                    chi.insert(inner->id);
                    des.insert(inner->id);
                    for(auto num : pkb->getDescendantsOf(inner->id))
                        des.insert(num);
                }

                for(auto inner : if_stmt->false_case.statements)
                {
                    chi.insert(inner->id);
                    des.insert(inner->id);
                    for(auto num : pkb->getDescendantsOf(inner->id))
                        des.insert(num);
                }
            }
            else if(s_ast::WhileLoop* while_stmt = dynamic_cast<s_ast::WhileLoop*>(stmt))
            {
                processDescendants(pkb, &while_stmt->body);

                for(auto inner : while_stmt->body.statements)
                {
                    chi.insert(inner->id);
                    des.insert(inner->id);
                    for(auto num : pkb->getDescendantsOf(inner->id))
                        des.insert(num);
                }
            }
            else
            {
                continue;
            }

            pkb->_direct_children[stmt->id] = chi;
            pkb->_descendants[stmt->id] = des;
        }
    }

    bool ProgramKB::isParent(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(_direct_parents.count(snd) == 0)
            return false;

        return _direct_parents[snd] == fst;
    }

    bool ProgramKB::isParentT(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(_ancestors.count(snd) == 0)
            return false;

        return _ancestors[snd].count(fst);
    }

    std::optional<s_ast::StatementNum> ProgramKB::getParentOf(s_ast::StatementNum fst)
    {
        // this will return 0 if it has no parent
        if(_direct_parents.count(fst) == 0)
            return std::nullopt;

        return _direct_parents[fst];
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getAncestorsOf(s_ast::StatementNum fst)
    {
        if(_ancestors.count(fst) == 0)
            return {};

        return _ancestors[fst];
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getChildrenOf(s_ast::StatementNum fst)
    {
        if(_direct_children.count(fst) == 0)
            return {};

        return _direct_children[fst];
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getDescendantsOf(s_ast::StatementNum fst)
    {
        if(_descendants.count(fst) == 0)
            return {};

        return _descendants[fst];
    }

    bool ProgramKB::followsRelationExists()
    {
        return this->m_follows_exists;
    }

    bool ProgramKB::parentRelationExists()
    {
        return this->m_parent_exists;
    }


    /**
     * End of Parent methods
     */

    /**
     * Start of Uses and Modifies methods
     */

    static void processExpr(ProgramKB* pkb, s_ast::Expr* expr, s_ast::Stmt* parent_stmt, s_ast::Procedure* parent_proc)
    {
        if(auto vr = dynamic_cast<s_ast::VarRef*>(expr))
        {
            pkb->uses_modifies.variables[vr->name].used_by.insert(parent_stmt);
            pkb->uses_modifies.variables[vr->name].used_by_procs.insert(parent_proc);

            pkb->uses_modifies.procedures[parent_proc->name].uses.insert(vr->name);

            pkb->uses_modifies.statements[parent_stmt->id - 1]->uses.insert(vr->name);
        }
        else if(dynamic_cast<s_ast::Constant*>(expr))
        {
            // do nothing
        }
        else if(auto bo = dynamic_cast<s_ast::BinaryOp*>(expr))
        {
            processExpr(pkb, bo->lhs, parent_stmt, parent_proc);
            processExpr(pkb, bo->rhs, parent_stmt, parent_proc);
        }
        else if(auto uo = dynamic_cast<s_ast::UnaryOp*>(expr))
        {
            processExpr(pkb, uo->expr, parent_stmt, parent_proc);
        }
        else
        {
            throw util::PkbException("pkb", "unknown expression type");
        }
    }

    static void processStmtList(ProgramKB* pkb, s_ast::StmtList* list, s_ast::Procedure* parent_proc);
    static void processStmt(ProgramKB* pkb, s_ast::Stmt* stmt, s_ast::Procedure* parent_proc)
    {
        if(auto i = dynamic_cast<s_ast::IfStmt*>(stmt))
        {
            processStmtList(pkb, &i->true_case, parent_proc);
            processStmtList(pkb, &i->false_case, parent_proc);

            processExpr(pkb, i->condition, i, parent_proc);
        }
        else if(auto w = dynamic_cast<s_ast::WhileLoop*>(stmt))
        {
            processStmtList(pkb, &w->body, parent_proc);

            processExpr(pkb, w->condition, w, parent_proc);
        }
        else if(auto a = dynamic_cast<s_ast::AssignStmt*>(stmt)) // prolly add the uses and modifies inside
        {
            pkb->uses_modifies.variables[a->lhs].modified_by.insert(stmt);
            pkb->uses_modifies.variables[a->lhs].modified_by_procs.insert(parent_proc);

            pkb->uses_modifies.procedures[parent_proc->name].modifies.insert(a->lhs);

            pkb->uses_modifies.statements[a->id - 1]->modifies.insert(a->lhs);

            processExpr(pkb, a->rhs, a, parent_proc);
        }
        else if(auto r = dynamic_cast<s_ast::ReadStmt*>(stmt))
        {
            pkb->uses_modifies.variables[r->var_name].modified_by.insert(stmt);
            pkb->uses_modifies.variables[r->var_name].modified_by_procs.insert(parent_proc);

            pkb->uses_modifies.statements[r->id - 1]->modifies.insert(r->var_name);

            pkb->uses_modifies.procedures[parent_proc->name].modifies.insert(r->var_name);
        }
        else if(auto p = dynamic_cast<s_ast::PrintStmt*>(stmt))
        {
            pkb->uses_modifies.variables[p->var_name].used_by.insert(stmt);
            pkb->uses_modifies.variables[p->var_name].used_by_procs.insert(parent_proc);

            pkb->uses_modifies.statements[p->id - 1]->uses.insert(p->var_name);

            pkb->uses_modifies.procedures[parent_proc->name].uses.insert(p->var_name);
        }
        else if(auto c = dynamic_cast<s_ast::ProcCall*>(stmt))
        {
            pkb->uses_modifies.procedures[parent_proc->name].calls.insert(c->proc_name);
            pkb->uses_modifies.procedures[c->proc_name].called_by.insert(parent_proc->name);
        }
        else
        {
            throw util::PkbException("pkb", "unknown statement type");
        }
    }

    static void processStmtList(ProgramKB* pkb, s_ast::StmtList* list, s_ast::Procedure* parent_proc)
    {
        for(const auto& stmt : list->statements)
            processStmt(pkb, stmt, parent_proc);
    }

    // Secondary processing step to fully populate uses and modifies for nested if/while, and proc call statements.
    static void reprocessStmtList(ProgramKB* pkb, s_ast::StmtList* list, s_ast::Procedure* proc)
    {
        for(const auto& stmt : list->statements)
        {
            if(auto i = dynamic_cast<s_ast::IfStmt*>(stmt))
            {
                reprocessStmtList(pkb, &i->true_case, proc);
                reprocessStmtList(pkb, &i->false_case, proc);

                for(const auto& child_stmt : i->true_case.statements)
                {
                    auto& tmp = pkb->uses_modifies.statements.at(child_stmt->id - 1);
                    pkb->uses_modifies.statements.at(stmt->id - 1)
                        ->modifies.insert(tmp->modifies.begin(), tmp->modifies.end());
                    pkb->uses_modifies.statements.at(stmt->id - 1)->uses.insert(tmp->uses.begin(), tmp->uses.end());
                }
                for(const auto& child_stmt : i->false_case.statements)
                {
                    auto& tmp = pkb->uses_modifies.statements.at(child_stmt->id - 1);
                    pkb->uses_modifies.statements.at(stmt->id - 1)
                        ->modifies.insert(tmp->modifies.begin(), tmp->modifies.end());
                    pkb->uses_modifies.statements.at(stmt->id - 1)->uses.insert(tmp->uses.begin(), tmp->uses.end());
                }
                for(auto& var : pkb->uses_modifies.statements.at(i->id - 1)->uses)
                {
                    pkb->uses_modifies.variables.at(var).used_by.insert(i);
                }
                for(auto& var : pkb->uses_modifies.statements.at(i->id - 1)->modifies)
                {
                    pkb->uses_modifies.variables.at(var).modified_by.insert(i);
                }
            }
            else if(auto w = dynamic_cast<s_ast::WhileLoop*>(stmt))
            {
                reprocessStmtList(pkb, &w->body, proc);

                for(const auto& child_stmt : w->body.statements)
                {
                    auto& tmp = pkb->uses_modifies.statements.at(child_stmt->id - 1);
                    pkb->uses_modifies.statements.at(stmt->id - 1)
                        ->modifies.insert(tmp->modifies.begin(), tmp->modifies.end());
                    pkb->uses_modifies.statements.at(stmt->id - 1)->uses.insert(tmp->uses.begin(), tmp->uses.end());
                }
                for(auto& var : pkb->uses_modifies.statements.at(w->id - 1)->uses)
                {
                    pkb->uses_modifies.variables.at(var).used_by.insert(w);
                }
                for(auto& var : pkb->uses_modifies.statements.at(w->id - 1)->modifies)
                {
                    pkb->uses_modifies.variables.at(var).modified_by.insert(w);
                }
            }
            else if(auto c = dynamic_cast<s_ast::ProcCall*>(stmt))
            {
                auto& tmp = pkb->uses_modifies.procedures.at(c->proc_name);
                // This is an inefficient quick fix, will change to callgraph bottom-up traversal once im free
                reprocessStmtList(pkb, &tmp.ast_proc->body, tmp.ast_proc);
                pkb->uses_modifies.statements.at(c->id - 1)->modifies.insert(tmp.modifies.begin(), tmp.modifies.end());
                pkb->uses_modifies.statements.at(c->id - 1)->uses.insert(tmp.uses.begin(), tmp.uses.end());

                for(auto& var : pkb->uses_modifies.procedures.at(c->proc_name).uses)
                {
                    pkb->uses_modifies.variables.at(var).used_by.insert(c);
                    pkb->uses_modifies.variables.at(var).used_by_procs.insert(proc);
                }
                for(auto& var : pkb->uses_modifies.procedures.at(c->proc_name).modifies)
                {
                    pkb->uses_modifies.variables.at(var).modified_by.insert(c);
                    pkb->uses_modifies.variables.at(var).modified_by_procs.insert(proc);
                }

                auto& stmt_rs = pkb->uses_modifies.statements.at(stmt->id - 1);
                pkb->uses_modifies.procedures.at(proc->name).uses.insert(stmt_rs->uses.begin(), stmt_rs->uses.end());
                pkb->uses_modifies.procedures.at(proc->name)
                    .modifies.insert(stmt_rs->modifies.begin(), stmt_rs->modifies.end());
            }
        }
    }

    bool UsesModifies::isUses(const simple::ast::StatementNum& stmt_num, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end() || stmt_num > this->statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        auto& stmt = this->statements.at(stmt_num - 1);
        if(auto c = dynamic_cast<s_ast::ProcCall*>(stmt->stmt))
        {
            return this->procedures[c->proc_name].uses.count(var) > 0;
        }
        else
        {
            return stmt->uses.count(var) > 0;
        }
    }

    bool UsesModifies::isUses(const std::string& proc, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end() || this->procedures.find(proc) == this->procedures.end())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        return this->procedures.at(proc).uses.count(var) > 0;
    }

    std::unordered_set<std::string> UsesModifies::getUsesVars(const simple::ast::StatementNum& stmt_num)
    {
        if(stmt_num > this->statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid statement number.");
        }
        return this->statements.at(stmt_num - 1)->uses;
    }

    std::unordered_set<std::string> UsesModifies::getUsesVars(const std::string& var)
    {
        if(this->procedures.find(var) == this->procedures.end())
        {
            throw util::PkbException("pkb::eval", "Procedure not found.");
        }
        return this->procedures.at(var).uses;
    }

    std::unordered_set<std::string> UsesModifies::getUses(const pql::ast::DESIGN_ENT& type, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end())
        {
            throw util::PkbException("pkb::eval", "Variable not found.");
        }
        std::unordered_set<std::string> uses;
        auto& stmt_list = this->variables.at(var).used_by;
        switch(type)
        {
            case pql::ast::DESIGN_ENT::ASSIGN:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::AssignStmt*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PRINT:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::PrintStmt*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::STMT:
                for(auto& stmt : stmt_list)
                    uses.insert(std::to_string(stmt->id));

                break;
            case pql::ast::DESIGN_ENT::CALL:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::ProcCall*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::IF:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::IfStmt*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::WHILE:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::WhileLoop*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PROCEDURE:
                for(auto& proc : this->variables.at(var).used_by_procs)
                    uses.insert(proc->name);
                break;
            default:
                throw util::PkbException("pkb::eval", "Invalid statement type.");
        }
        return uses;
    }

    bool UsesModifies::isModifies(const simple::ast::StatementNum& stmt_num, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end() || stmt_num > this->statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        auto& stmt = this->statements.at(stmt_num - 1);
        if(auto c = dynamic_cast<s_ast::ProcCall*>(stmt->stmt))
        {
            return this->procedures[c->proc_name].modifies.count(var) > 0;
        }
        else
        {
            return stmt->modifies.count(var) > 0;
        }
    }

    bool UsesModifies::isModifies(const std::string& proc, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end() || this->procedures.find(proc) == this->procedures.end())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        return this->procedures.at(proc).modifies.count(var) > 0;
    }

    std::unordered_set<std::string> UsesModifies::getModifiesVars(const simple::ast::StatementNum& stmt_num)
    {
        if(stmt_num > this->statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid statement number.");
        }
        return this->statements.at(stmt_num - 1)->modifies;
    }

    std::unordered_set<std::string> UsesModifies::getModifiesVars(const std::string& var)
    {
        if(this->procedures.find(var) == this->procedures.end())
        {
            throw util::PkbException("pkb::eval", "Procedure {} not found.", var);
        }
        return this->procedures.at(var).modifies;
    }

    std::unordered_set<std::string> UsesModifies::getModifies(const pql::ast::DESIGN_ENT& type, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end())
        {
            throw util::PkbException("pkb::eval", "Variable not found.");
        }
        std::unordered_set<std::string> modifies;
        auto& stmt_list = this->variables.at(var).modified_by;
        switch(type)
        {
            case pql::ast::DESIGN_ENT::ASSIGN:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::AssignStmt*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::READ:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::ReadStmt*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::STMT:
                for(auto& stmt : stmt_list)
                {
                    modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::CALL:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::ProcCall*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::IF:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::IfStmt*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::WHILE:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::WhileLoop*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PROCEDURE:
                for(auto& proc : this->variables.at(var).modified_by_procs)
                    modifies.insert(proc->name);

                break;
            default:
                throw util::PkbException("pkb::eval", "Invalid statement type.");
        }
        return modifies;
    }

    /**
     * End of Uses and Modifies methods
     */

    Result<ProgramKB*> processProgram(s_ast::Program* program)
    {
        auto pkb = new ProgramKB();

        for(const auto& proc : program->procedures)
        {
            processCallGraph(pkb, &proc->body, &proc->name);
        }

        if(pkb->proc_calls.cycleExists())
            return ErrFmt("Cyclic or recursive calls are not allowed");
        if(auto a = pkb->proc_calls.missingProc(program->procedures); a != "")
            return ErrFmt("Procedure '{}' is undefined", a);

        // do a first pass to number all the statements, set the
        // parent stmtlist, and collect all the procedures.
        for(const auto& proc : program->procedures)
        {
            collectStmtList(pkb, &proc->body);

            if(pkb->uses_modifies.procedures.find(proc->name) != pkb->uses_modifies.procedures.end())
                throw util::PkbException("pkb", "procedure '{}' is already defined", proc->name);

            pkb->uses_modifies.procedures[proc->name].ast_proc = proc;
        }

        // do a second pass to populate the follows vector and parent, uses, modifies hashmap.
        for(const auto& proc : program->procedures)
        {
            processFollows(pkb, &proc->body);
            processStmtList(pkb, &proc->body, proc);
        }

        // do a third pass to populate the ancestors/descendants hashmap and
        // to populate uses/modifies for nested if/while statements
        for(const auto& proc : program->procedures)
        {
            processAncestors(pkb, &proc->body);
            reprocessStmtList(pkb, &proc->body, proc);
            processDescendants(pkb, &proc->body);
        }

        return Ok(pkb);
    }
}
