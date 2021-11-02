// cfg.cpp

#include "pkb.h"
#include "exceptions.h"

#include <zpr.h>
#include <queue>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <iostream>
#define INF SIZE_MAX

namespace pkb
{
    using StatementNum = simple::ast::StatementNum;

    static void check_in_range(StatementNum num, size_t max)
    {
        if(num > max || num <= 0)
            throw util::PkbException("pkb", "StatementNum is out of range");
    }

    CFG::CFG(const ProgramKB* pkb, size_t v) : m_pkb(pkb)
    {
        total_inst = v;
        adj_mat = new size_t*[v];
        adj_mat_bip = new size_t*[v];

        m_next_exists = false;

        for(size_t i = 0; i < v; i++)
        {
            this->adj_mat[i] = new size_t[v];
            this->adj_mat_bip[i] = new size_t[v];
            for(size_t j = 0; j < v; j++)
            {
                adj_mat[i][j] = INF;
                adj_mat_bip[i][j] = INF;
            }
        }
    }

    CFG::~CFG()
    {
        for(size_t i = 0; i < total_inst; i++)
        {
            delete[] this->adj_mat[i];
            delete[] this->adj_mat_bip[i];
        }

        delete[] this->adj_mat;
        delete[] this->adj_mat_bip;
    }

    void CFG::addEdge(StatementNum stmt1, StatementNum stmt2)
    {
        check_in_range(stmt1, total_inst);
        check_in_range(stmt2, total_inst);
        adj_mat[stmt1 - 1][stmt2 - 1] = 1;
        m_next_exists = true;

        if(!adj_lst.count(stmt1))
        {
            StatementSet stmts { stmt2 };
            adj_lst[stmt1] = stmts;
        }
        else
        {
            adj_lst[stmt1].insert(stmt2);
        }
    }
    // weight here refers to edge label + 1
    void CFG::addEdgeBip(StatementNum stmt1, StatementNum stmt2, size_t weight)
    {
        check_in_range(stmt1, total_inst);
        check_in_range(stmt2, total_inst);
        spa_assert(weight != 0);
        auto pair = std::make_pair(stmt1, stmt2);
        if(weight == INF) // we are removing edge
        {
            if(adj_mat_bip[stmt1 - 1][stmt2 - 1] == 0)
            {
                bip_ref[pair].erase(weight);
                if(bip_ref[pair].size() == 1)
                {
                    int a = *bip_ref[pair].begin();
                    adj_mat_bip[stmt1 - 1][stmt2 - 1] = a;
                    bip_ref.erase(pair);
                }
            }
            else if(adj_mat_bip[stmt1 - 1][stmt2 - 1] != INF)
            {
                adj_mat_bip[stmt1 - 1][stmt2 - 1] = INF;
            }
            if(adj_lst_bip.count(stmt1) != 0)
            {
                size_t idx = 0;
                for(auto a : adj_lst_bip[stmt1])
                {
                    if(a.first == stmt2 && a.second == 1)
                    {
                        adj_lst_bip[stmt1].erase(adj_lst_bip[stmt1].begin() + idx);
                        break;
                    }
                    idx++;
                }
                if(adj_lst_bip[stmt1].size() == 0)
                {
                    adj_lst_bip.erase(stmt1);
                }
            }
        }
        else
        {
            if(adj_mat_bip[stmt1 - 1][stmt2 - 1] == INF)
            {
                adj_mat_bip[stmt1 - 1][stmt2 - 1] = weight;
            }
            else if(adj_mat_bip[stmt1 - 1][stmt2 - 1] == 0)
            {
                bip_ref[pair].insert(weight);
            }
            else
            {
                bip_ref[pair] = { adj_mat_bip[stmt1 - 1][stmt2 - 1], weight };
                adj_mat_bip[stmt1 - 1][stmt2 - 1] = 0;
            }
            if(adj_lst_bip.count(stmt1) == 0)
            {
                std::vector<std::pair<StatementNum, size_t>> stmts { std::make_pair(stmt2, weight) };
                adj_lst_bip[stmt1] = stmts;
            }
            else
            {
                adj_lst_bip[stmt1].push_back(std::make_pair(stmt2, weight));
            }
        }
    }

    bool CFG::nextRelationExists() const
    {
        return m_next_exists;
    }

    std::string CFG::getMatRep(int i) const
    {
        size_t** mat;
        switch(i)
        {
            case 1:
                mat = adj_mat;
                break;
            case 2:
                mat = adj_mat_bip;
                break;
            default:
                mat = adj_mat;
                break;
        }
        auto res = zpr::sprint("      ");
        for(size_t i = 0; i < total_inst; i++)
        {
            res += zpr::sprint("{03} ", i + 1);
        }
        res += zpr::sprint("\n");

        for(size_t i = 0; i < total_inst; i++)
        {
            res += zpr::sprint("{03} | ", i + 1);
            for(size_t j = 0; j < total_inst; j++)
            {
                res += zpr::sprint("{03} ", mat[i][j] == INF ? 999 : mat[i][j]);
            }
            res += zpr::sprint("\n");
        }

        return res;
    }

    void CFG::computeDistMat()
    {
        // Adapted from https://www.geeksforgeeks.org/floyd-warshall-algorithm-dp-16/
        for(size_t k = 0; k < total_inst; k++)
        {
            for(size_t i = 0; i < total_inst; i++)
            {
                for(size_t j = 0; j < total_inst; j++)
                {
                    if(adj_mat[i][j] > (adj_mat[i][k] + adj_mat[k][j]) &&
                        (adj_mat[k][j] != INF && adj_mat[i][k] != INF))
                        adj_mat[i][j] = adj_mat[i][k] + adj_mat[k][j];
                }
            }
        }
    }

    void CFG::addAssignStmtMapping(StatementNum id, Statement* stmt)
    {
        assign_stmts[id] = stmt;
    }

    void CFG::addCallStmtMapping(StatementNum id, Statement* stmt)
    {
        call_stmts[id] = stmt;
    }

    void CFG::addModStmtMapping(StatementNum id, Statement* stmt)
    {
        mod_stmts[id] = stmt;
    }

    const Statement* CFG::getAssignStmtMapping(StatementNum id) const
    {
        if(auto it = assign_stmts.find(id); it != assign_stmts.end())
            return it->second;

        return nullptr;
    }

    const Statement* CFG::getCallStmtMapping(StatementNum id) const
    {
        if(call_stmts.count(id) == 0)
            return nullptr;
        return call_stmts.at(id);
    }

    const Statement* CFG::getModStmtMapping(StatementNum id) const
    {
        if(auto it = mod_stmts.find(id); it != mod_stmts.end())
            return it->second;

        return nullptr;
    }

    bool CFG::isStatementNext(StatementNum stmt1, StatementNum stmt2) const
    {
        check_in_range(stmt1, total_inst);
        check_in_range(stmt2, total_inst);
        return adj_mat[stmt1 - 1][stmt2 - 1] == 1;
    }

    bool CFG::isStatementTransitivelyNext(StatementNum stmt1, StatementNum stmt2) const
    {
        check_in_range(stmt1, total_inst);
        check_in_range(stmt2, total_inst);
        return adj_mat[stmt1 - 1][stmt2 - 1] < INF; // impossible to be 0 since no recursive call
    }


    const StatementSet& CFG::getNextStatements(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetNextStatements(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        if(auto it = adj_lst.find(id); it != adj_lst.end())
            ret = it->second;

        return stmt.cacheNextStatements(std::move(ret));
    }

    const StatementSet& CFG::getTransitivelyNextStatements(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);

        if(auto cache = stmt.maybeGetTransitivelyNextStatements(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(size_t j = 0; j < total_inst; j++)
        {
            if(adj_mat[id - 1][j] < INF)
                ret.insert(j + 1);
        }

        return stmt.cacheTransitivelyNextStatements(std::move(ret));
    }

    const StatementSet& CFG::getPreviousStatements(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetPreviousStatements(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(size_t i = 0; i < this->total_inst; i++)
        {
            if(adj_mat[i][id - 1] == 1)
                ret.insert(i + 1);
        }

        return stmt.cachePreviousStatements(std::move(ret));
    }

    const StatementSet& CFG::getTransitivelyPreviousStatements(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);

        if(auto cache = stmt.maybeGetTransitivelyPreviousStatements(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(size_t i = 0; i < this->total_inst; i++)
        {
            if(adj_mat[i][id - 1] < INF)
                ret.insert(i + 1);
        }

        return stmt.cacheTransitivelyPreviousStatements(std::move(ret));
    }

    bool CFG::doesAffect(StatementNum id1, StatementNum id2) const
    {
        if(!isStatementTransitivelyNext(id1, id2))
            return false;

        auto stmt1 = getAssignStmtMapping(id1);
        auto stmt2 = getAssignStmtMapping(id2);

        if(stmt1 == nullptr || stmt2 == nullptr)
            return false;

        const auto& modified = stmt1->getModifiedVariables();
        const auto& used = stmt2->getUsedVariables();

        std::unordered_set<std::string> vars;
        for(const auto& var : modified)
        {
            if(used.count(var))
                vars.insert(var);
        }

        StatementSet visited;
        std::queue<std::pair<StatementNum, std::string>> q;
        for(auto stmt : adj_lst.at(id1))
        {
            for(auto var : vars)
                q.emplace(stmt, var);
            visited.insert(stmt);
        }

        while(!q.empty())
        {
            auto [num, var] = q.front();
            q.pop();

            if(num == id2)
                return true;
            if(getModStmtMapping(num) != nullptr && getModStmtMapping(num)->modifiesVariable(var))
                continue;

            if(auto it = adj_lst.find(num); it != adj_lst.end())
            {
                for(auto stmt : it->second)
                {
                    if(visited.count(stmt))
                        continue;
                    visited.insert(stmt);
                    q.emplace(stmt, var);
                }
            }
        }

        return false;
    }

    bool CFG::doesTransitivelyAffect(StatementNum id1, StatementNum id2) const
    {
        StatementSet visited;
        std::queue<StatementNum> q;
        for(auto stmt : getAffectedStatements(id1))
        {
            q.push(stmt);
            visited.insert(stmt);
        }

        while(!q.empty())
        {
            auto num = q.front();
            q.pop();
            if(num == id2)
                return true;

            for(auto stmt : getAffectedStatements(num))
            {
                if(visited.count(stmt) == 0)
                    q.push(stmt);
                visited.insert(stmt);
            }
        }
        return false;
    }

    bool CFG::affectsRelationExists() const
    {
        // TODO: is there a cheaper way of doing this?
        for(auto [assid, _] : this->assign_stmts)
        {
            if(!this->getAffectedStatements(assid).empty())
                return true;
        }

        return false;
    }

    const StatementSet& CFG::getAffectedStatements(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetAffectedStatements(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(auto stmt : getTransitivelyNextStatements(id))
            if(doesAffect(id, stmt))
                ret.insert(stmt);

        return stmt.cacheAffectedStatements(std::move(ret));
    }

    const StatementSet& CFG::getAffectingStatements(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetAffectingStatements(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(auto stmt : getTransitivelyPreviousStatements(id))
            if(doesAffect(stmt, id))
                ret.insert(stmt);
        return stmt.cacheAffectingStatements(std::move(ret));
    }

    const StatementSet& CFG::getTransitivelyAffectedStatements(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetTransitivelyAffectedStatements(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(auto stmt : getTransitivelyNextStatements(id))
            if(doesTransitivelyAffect(id, stmt))
                ret.insert(stmt);
        return stmt.cacheTransitivelyAffectedStatements(std::move(ret));
    }

    const StatementSet& CFG::getTransitivelyAffectingStatements(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetTransitivelyAffectingStatements(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(auto stmt : getTransitivelyPreviousStatements(id))
            if(doesTransitivelyAffect(stmt, id))
                ret.insert(stmt);
        return stmt.cacheTransitivelyAffectingStatements(std::move(ret));
    }

    bool CFG::isStatementNextBip(StatementNum stmt1, StatementNum stmt2) const
    {
        check_in_range(stmt1, total_inst);
        check_in_range(stmt2, total_inst);
        return adj_mat_bip[stmt1 - 1][stmt2 - 1] != INF;
    }

    bool CFG::isStatementTransitivelyNextBip(StatementNum id1, StatementNum id2) const
    {
        auto& stmt1 = m_pkb->getStatementAt(id1);
        auto& stmt2 = m_pkb->getStatementAt(id2);

        std::set<StatementNum> callStack {};
        StatementSet visited;

        auto currProc = stmt1.getProc();
        for(auto& i : m_pkb->maybeGetProcedureNamed(currProc->name)->getAllTransitiveCallers())
        {
            for(auto a : m_pkb->maybeGetProcedureNamed(i)->getCallStmts())
            {
                callStack.insert(a);
            }
        }
        for(auto a : m_pkb->maybeGetProcedureNamed(currProc->name)->getCallStmts())
        {
            callStack.insert(a);
        }

        std::queue<StatementNum> q {};
        q.emplace(id1);
        bool initialNode = true;
        while(!q.empty())
        {
            auto next = q.front();
            if(next == id2 && !initialNode)
                return true;
            if(initialNode)
                initialNode = false;
            auto callStmt = getCallStmtMapping(next);
            if(callStmt != nullptr)
            {
                callStack.insert(next);
                for(auto a : gates.at(callStmt->getProc()->name).second)
                {
                    q.emplace(a);
                }
            }
            if(auto it = adj_lst_bip.find(next); it != adj_lst_bip.end())
            {
                for(auto& pair : it->second)
                {
                    if(pair.second == 1 || callStack.count(pair.second - 1) != 0)
                    {
                        if(visited.count(pair.first) == 0)
                        {
                            q.emplace(pair.first);
                        }
                    }
                }
            }
            visited.insert(next);
            q.pop();
        }
        return false;
    }

    const StatementSet& CFG::getNextStatementsBip(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetNextStatementsBip(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        if(auto it = adj_lst_bip.find(id); it != adj_lst_bip.end())
        {
            for(auto& pair : it->second)
            {
                ret.insert(pair.first);
            }
        }
        return stmt.cacheNextStatementsBip(std::move(ret));
    }

    const StatementSet& CFG::getTransitivelyNextStatementsBip(StatementNum id) const
    {
        // kinda similar. any ways to reduce code dup.
        auto& stmt = m_pkb->getStatementAt(id);

        if(auto cache = stmt.maybeGetTransitivelyNextStatementsBip(); cache != nullptr)
            return *cache;

        std::set<StatementNum> callStack {};
        StatementSet visited;

        auto currProc = stmt.getProc();
        for(auto& i : m_pkb->maybeGetProcedureNamed(currProc->name)->getAllTransitiveCallers())
        {
            for(auto a : m_pkb->maybeGetProcedureNamed(i)->getCallStmts())
            {
                callStack.insert(a);
            }
        }
        for(auto a : m_pkb->maybeGetProcedureNamed(currProc->name)->getCallStmts())
        {
            callStack.insert(a);
        }

        std::queue<StatementNum> q {};
        q.emplace(id);
        bool initialNode = true;
        while(!q.empty())
        {
            auto next = q.front();
            auto callStmt = getCallStmtMapping(next);
            if(callStmt != nullptr)
            {
                callStack.insert(next);
                for(auto a : gates.at(callStmt->getProc()->name).second)
                {
                    q.emplace(a);
                }
            }
            if(auto it = adj_lst_bip.find(next); it != adj_lst_bip.end())
            {
                for(auto& pair : it->second)
                {
                    if(pair.second == 1 || callStack.count(pair.second - 1) != 0)
                    {
                        if(visited.count(pair.first) == 0)
                        {
                            q.emplace(pair.first);
                        }
                    }
                }
            }
            if(!initialNode)
                visited.insert(next);
            if(initialNode)
                initialNode = false;
            q.pop();
        }

        return stmt.cacheTransitivelyNextStatementsBip(std::move(visited));
    }
    const StatementSet& CFG::getPreviousStatementsBip(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetPreviousStatementsBip(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(size_t i = 0; i < this->total_inst; i++)
        {
            if(adj_mat_bip[i][id - 1] < INF)
                ret.insert(i + 1);
        }

        return stmt.cachePreviousStatementsBip(std::move(ret));
    }
    const StatementSet& CFG::getTransitivelyPreviousStatementsBip(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);

        if(auto cache = stmt.maybeGetTransitivelyPreviousStatementsBip(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        
        for(size_t i = 0; i < total_inst; i++)
        {
            if(isStatementTransitivelyNextBip(i+1, id))
            {
                ret.insert(i+1);
            }
        }
        return stmt.cacheTransitivelyPreviousStatementsBip(std::move(ret));
    }
}
