#include <cassert>
#include <queue>

#include <zpr.h>
#include <zst.h>

#include "ast.h"
#include "pkb.h"
#include "util.h"

namespace pkb
{
    using zst::Ok;
    using zst::Err;
    using zst::ErrFmt;

    template <typename T>
    using Result = zst::Result<T, std::string>;
    namespace s_ast = simple::ast;

    // collection only entails numbering the statements
    static void collectStmtList(ProgramKB* pkb, s_ast::StmtList* list);
    static void collectStmt(ProgramKB* pkb, s_ast::Stmt* stmt, s_ast::StmtList* parent)
    {
        // the statement should not have been seen yet.
        assert(stmt->id == 0);

        stmt->parent_list = parent;
        stmt->id = pkb->statements.size() + 1;
        pkb->statements.push_back(stmt);

        if(auto i = dynamic_cast<s_ast::IfStmt*>(stmt); i)
        {
            pkb->if_statements.push_back(stmt);
            collectStmtList(pkb, &i->true_case);
            collectStmtList(pkb, &i->false_case);
            i->true_case.parent_statement = stmt;
            i->false_case.parent_statement = stmt;
        }
        else if(auto w = dynamic_cast<s_ast::WhileLoop*>(stmt); w)
        {
            pkb->while_statements.push_back(stmt);
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
            throw "Key does not exist!";
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
            throw "Key does not exist!";
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
            }

            for(size_t k = 0; k < i; k++)
            {
                if(i - k == 1)
                {
                    follows->directly_before = stmt_list[k]->id;
                }
                follows->before.insert(stmt_list[k]->id);
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

    static void processCalls(ProgramKB* pkb, s_ast::StmtList* list, std::string* procName)
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
                processCalls(pkb, &i->true_case, procName);
                processCalls(pkb, &i->false_case, procName);
            }
            else if(auto w = dynamic_cast<s_ast::WhileLoop*>(stmt); w)
            {
                processCalls(pkb, &w->body, procName);
            }
        }
    }

    void Calls::addEdge(std::string a, std::string b)
    {
        adj[a].insert(b);
    }

    // returns an iterator to the dest node of the next edge after the removed edge
    std::unordered_set<std::string>::iterator Calls::removeEdge(
        std::string a, std::string b, std::unordered_map<std::string, std::unordered_set<std::string>>* tempAdj)
    {
        std::unordered_set<std::string>::iterator ret = (*tempAdj)[a].erase((*tempAdj)[a].find(b));
        return ret;
    }

    // runs dfs to detect cycle
    bool Calls::dfs(std::string a, std::unordered_map<std::string, std::unordered_set<std::string>>* tempAdj,
        std::unordered_set<std::string>* visited)
    {
        if(visited->find(a) != visited->end())
        {
            return true;
        }
        visited->insert(a);
        if(tempAdj->find(a) != tempAdj->end())
        {
            std::unordered_set<std::string>::iterator s = (*tempAdj)[a].begin();
            while(s != (*tempAdj)[a].end())
            {
                if(dfs(*s, tempAdj, visited))
                {
                    return true;
                }
                else
                {
                    s = removeEdge(a, *s, tempAdj);
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
    bool Calls::cycleExists()
    {
        std::unordered_map<std::string, std::unordered_set<std::string>> tempAdj;
        std::unordered_set<std::string> visited;
        auto i = adj.begin();
        while(i != adj.end())
        {
            for(auto a : i->second)
            {
                tempAdj[i->first].insert(a.c_str());
            }
            ++i;
        }
        while(tempAdj.size() != 0)
        {
            bool res = dfs(tempAdj.begin()->first, &tempAdj, &visited);
            if(res)
                return true;
        }
        return false;
    }

    // Takes in two 1-indexed StatementNums
    bool ProgramKB::isFollows(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        // thinking of more elegant ways of handling this hmm
        if(fst > this->follows.size() || snd > this->follows.size() || fst < 1 || snd < 1)
            util::error("pkb", "StatementNum out of range.");

        return this->follows[fst - 1]->directly_after == snd;
    }

    // Same as isFollows
    bool ProgramKB::isFollowsT(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(fst > this->follows.size() || snd > this->follows.size() || fst < 1 || snd < 1)
            util::error("pkb", "StatementNum out of range.");

        return this->follows[fst - 1]->after.count(snd) > 0;
    }

    // Takes in two 1-indexed StatementNums. Allows for 0 to be used as a wildcard on 1 of the parameters.
    std::unordered_set<s_ast::StatementNum> ProgramKB::getFollowsTList(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(fst > this->follows.size() || snd > this->follows.size() || fst < 0 || snd < 0)
            util::error("pkb", "StatementNum out of range.");
        if((fst < 1 && snd < 1) || (fst != 0 && snd != 0))
            util::error("pkb", "Only 1 wildcard is to be used.");

        if(fst == 0)
        {
            return this->follows[snd - 1]->before;
        }
        else if(snd == 0)
        {
            return this->follows[fst - 1]->after;
        }

        // Should not be reachable.
        util::error("pkb", "Unexpected error.");
    }

    // Start of parent methods

    // processes the direct parents of statements
    static void processParent(ProgramKB* pkb, s_ast::StmtList* list, s_ast::StatementNum par)
    {
        for(auto stmt : list->statements)
        {
            pkb->_direct_parents[stmt->id] = par;

            if(s_ast::IfStmt* if_stmt = dynamic_cast<s_ast::IfStmt*>(stmt))
            {
                processParent(pkb, &if_stmt->true_case, stmt->id);
                if_stmt->true_case.parent_statement = if_stmt;

                processParent(pkb, &if_stmt->false_case, stmt->id);
                if_stmt->false_case.parent_statement = if_stmt;
            }
            else if(s_ast::WhileLoop* while_stmt = dynamic_cast<s_ast::WhileLoop*>(stmt))
            {
                processParent(pkb, &while_stmt->body, stmt->id);
                while_stmt->body.parent_statement = while_stmt;
            }
        }
    }

    // processes the ancestors of statements
    static void processAncestors(ProgramKB* pkb, s_ast::StmtList* lst)
    {
        using StmtPair = std::pair<s_ast::Stmt*, s_ast::Stmt*>;
        std::queue<StmtPair> q;

        for(auto stmt : lst->statements)
            q.push({ stmt, nullptr });

        while(!q.empty())
        {
            auto [child, parent] = q.front();
            q.pop();

            if(child == nullptr)
                continue;

            std::unordered_set<s_ast::StatementNum> anc;
            if(pkb->_ancestors.count(child->id) == 0)
                pkb->_ancestors[child->id] = anc;

            anc = pkb->_ancestors[child->id];

            if(parent != nullptr)
            {
                std::unordered_set<s_ast::StatementNum> par;
                for(auto num : par)
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
        }
    }

    bool ProgramKB::isParent(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(_direct_parents.count(fst) == 0)
            return false;

        return _direct_parents[fst] == snd;
    }

    bool ProgramKB::isParentT(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(_ancestors.count(snd) == 0)
            return false;

        return _ancestors[snd].count(fst);
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getAncestorsOf(s_ast::StatementNum fst)
    {
        std::unordered_set<s_ast::StatementNum> anc;

        if(_ancestors.count(fst) == 0)
            return anc;

        return _ancestors[fst];
    }

    // End of parent methods

    Result<ProgramKB*> processProgram(s_ast::Program* program)
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

        // do a second pass to populate the follows vector and parent hashmap.
        for(const auto& proc : program->procedures)
        {
            processFollows(pkb, &proc->body);
            processParent(pkb, &proc->body, -1);
        }

        // do a third pass to populate the ancestors hashmap
        for(const auto& proc : program->procedures)
        {
            processAncestors(pkb, &proc->body);
        }

        for(const auto& proc : program->procedures)
        {
            processCalls(pkb, &proc->body, &proc->name);
        }

        if(pkb->proc_calls.cycleExists())
            return ErrFmt("Cyclic or recursive calls are not allowed");

        return Ok(pkb);
    }
}
