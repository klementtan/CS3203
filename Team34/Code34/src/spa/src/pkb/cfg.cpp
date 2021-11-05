// cfg.cpp

#include "pkb.h"
#include "exceptions.h"

#include <zpr.h>
#include <algorithm>
#include <queue>
#include <stack>
#include <unordered_set>
#include <vector>
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
                    size_t a = *bip_ref[pair].begin();
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
            m_next_bip_exists = true;
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

    bool CFG::nextBipRelationExists() const
    {
        return m_next_bip_exists;
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
                for(auto [proc_name,p] : gates)
                {
                    std::cout << proc_name << std::endl;
                    std::cout << p.first << std::endl;
                    for(auto a : p.second)
                    {
                        std::cout << a << "   ";
                    }
                    std::cout << std::endl;
                }
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

        std::string mod_var = *modified.begin();
        bool is_modified = false;
        for(const auto& var : used)
            is_modified |= var == mod_var;

        if(!is_modified)
            return false;

        StatementSet visited;
        std::queue<std::pair<StatementNum, std::string>> q;
        for(auto stmt : adj_lst.at(id1))
        {
            q.emplace(stmt, mod_var);
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

    bool CFG::doesAffectBip(StatementNum id1, StatementNum id2) const
    {
        if(!isStatementTransitivelyNextBip(id1, id2))
            return false;

        auto stmt1 = getAssignStmtMapping(id1);
        auto stmt2 = getAssignStmtMapping(id2);

        if(stmt1 == nullptr || stmt2 == nullptr)
            return false;

        const auto& modified = stmt1->getModifiedVariables();
        const auto& used = stmt2->getUsedVariables();

        std::string mod_var = *modified.begin();
        bool is_modified = false;
        for(const auto& var : used)
            is_modified |= var == mod_var;

        if(!is_modified)
            return false;

        StatementSet visited;
        std::queue<std::tuple<StatementNum, std::string, std::stack<StatementNum>>> q;
        for(auto [stmt, weight] : adj_lst_bip.at(id1))
        {
            std::stack<StatementNum> s;
            q.emplace(stmt, mod_var, s);
            visited.insert(stmt);
        }

        while(!q.empty())
        {
            auto [num, var, calls] = q.front();
            q.pop();

            if(num == id2)
                return true;

            if(getCallStmtMapping(num) == nullptr)
                if(getModStmtMapping(num) != nullptr && getModStmtMapping(num)->modifiesVariable(var))
                    continue;

            if(auto it = adj_lst_bip.find(num); it != adj_lst_bip.end())
            {
                for(auto [stmt, weight] : it->second)
                {
                    if(weight == 1)
                    {
                        if(visited.count(stmt))
                            continue;
                        q.emplace(stmt, var, calls);
                        visited.insert(stmt);
                    }
                    else
                    {
                        if(getCallStmtMapping(num) == nullptr)
                        {
                            std::stack<StatementNum> calls_copy(calls);
                            if(!calls_copy.empty())
                            {
                                if(calls.top() == weight)
                                {
                                    if(visited.count(stmt))
                                        continue;
                                    calls_copy.pop();
                                    q.emplace(stmt, var, calls_copy);
                                    visited.insert(stmt);
                                }
                            }
                            else
                            {
                                if(visited.count(stmt))
                                    continue;
                                q.emplace(stmt, var, calls_copy);
                                visited.insert(stmt);
                            }
                        }
                        else
                        {
                            std::stack<StatementNum> calls_copy(calls);
                            calls_copy.push(weight);
                            q.emplace(stmt, var, calls_copy);
                            visited.clear();
                            visited.insert(stmt);
                        }
                    }
                }
            }
        }

        return false;
    }

    bool CFG::doesTransitivelyAffectBip(StatementNum id1, StatementNum id2) const
    {
        std::queue<std::pair<StatementNum, std::queue<StatementNum>>> q;
        for(auto stmt : getAffectedStatementsBip(id1))
        {
            std::queue<StatementNum> path;
            path.push(stmt);
            q.emplace(stmt, path);
        }

        while(!q.empty())
        {
            auto [num, path] = q.front();
            q.pop();

            if(num == id2)
            {
                if(isValidTransitivelyAffectBip(id1, id2, path))
                    return true;
                else
                    continue;
            }

            for(auto stmt : getAffectedStatementsBip(num))
            {
                std::queue<StatementNum> path_copy(path);
                path_copy.push(stmt);
                q.emplace(stmt, path_copy);
            }
        }

        return false;
    }

    bool CFG::isValidTransitivelyAffectBip(StatementNum id1, StatementNum id2, std::queue<StatementNum> path) const
    {
        std::queue<std::tuple<StatementNum, std::queue<StatementNum>, std::stack<StatementNum>>> q;
        for(auto [stmt, weight] : adj_lst_bip.at(id1))
        {
            std::queue<StatementNum> path_copy(path);
            std::stack<StatementNum> s;
            q.emplace(stmt, path_copy, s);
        }

        while(!q.empty())
        {
            auto [num, transits, calls] = q.front();
            q.pop();

            if(!transits.empty() && transits.front() == num)
                transits.pop();
            if(transits.empty())
                return true;

            if(auto it = adj_lst_bip.find(num); it != adj_lst_bip.end())
            {
                for(auto [stmt, weight] : it->second)
                {
                    if(weight == 1)
                    {
                        q.emplace(stmt, transits, calls);
                    }
                    else
                    {
                        if(getCallStmtMapping(num) == nullptr)
                        {
                            std::stack<StatementNum> calls_copy(calls);
                            if(!calls_copy.empty())
                            {
                                if(calls.top() == weight)
                                {
                                    calls_copy.pop();
                                    q.emplace(stmt, transits, calls_copy);
                                }
                            }
                            else
                            {
                                q.emplace(stmt, transits, calls_copy);
                            }
                        }
                        else
                        {
                            std::stack<StatementNum> calls_copy(calls);
                            calls_copy.push(weight);
                            q.emplace(stmt, transits, calls_copy);
                        }
                    }
                }
            }
        }

        return false;
    }

    bool CFG::affectsRelationExists() const
    {
        // TODO: is there a cheaper way of doing this?
        for(auto [assid, _] : this->assign_stmts)
        {
            if(this->getAffectedStatements(assid).size() > 0)
                return true;
        }

        return false;
    }

    bool CFG::affectsBipRelationExists() const
    {
        // TODO: is there a cheaper way of doing this?
        for(auto [assid, _] : this->assign_stmts)
        {
            if(this->getAffectedStatementsBip(assid).size() > 0)
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

    const StatementSet& CFG::getAffectedStatementsBip(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetAffectedStatementsBip(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(auto stmt : getTransitivelyNextStatementsBip(id))
            if(doesAffectBip(id, stmt))
                ret.insert(stmt);

        return stmt.cacheAffectedStatementsBip(std::move(ret));
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

    const StatementSet& CFG::getAffectingStatementsBip(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetAffectingStatementsBip(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(auto stmt : getTransitivelyPreviousStatementsBip(id))
            if(doesAffectBip(stmt, id))
                ret.insert(stmt);
        return stmt.cacheAffectingStatementsBip(std::move(ret));
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

    const StatementSet& CFG::getTransitivelyAffectedStatementsBip(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetTransitivelyAffectedStatementsBip(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(auto stmt : getTransitivelyNextStatementsBip(id))
            if(doesTransitivelyAffectBip(id, stmt))
                ret.insert(stmt);
        return stmt.cacheTransitivelyAffectedStatementsBip(std::move(ret));
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

    const StatementSet& CFG::getTransitivelyAffectingStatementsBip(StatementNum id) const
    {
        auto& stmt = m_pkb->getStatementAt(id);
        if(auto cache = stmt.maybeGetTransitivelyAffectingStatementsBip(); cache != nullptr)
            return *cache;

        StatementSet ret {};
        for(auto stmt : getTransitivelyPreviousStatementsBip(id))
            if(doesTransitivelyAffectBip(stmt, id))
                ret.insert(stmt);
        return stmt.cacheTransitivelyAffectingStatementsBip(std::move(ret));
    }

    bool CFG::isStatementNextBip(StatementNum stmt1, StatementNum stmt2) const
    {
        check_in_range(stmt1, total_inst);
        check_in_range(stmt2, total_inst);
        return adj_mat_bip[stmt1 - 1][stmt2 - 1] != INF;
    }

    StatementSet CFG::getCurrentStack(const StatementNum id) const
    {
        auto& stmt= m_pkb->getStatementAt(id);
        StatementSet callStack {};

        auto currProc = stmt.getProc();
        for(auto& i : m_pkb->maybeGetProcedureNamed(currProc->name)->getAllTransitiveCallers())
        {
            auto& callers = m_pkb->getProcedureNamed(i).getCallStmts();
            callStack.insert(callers.begin(), callers.end());
        }
        auto& callers = m_pkb->getProcedureNamed(currProc->name).getCallStmts();
        callStack.insert(callers.begin(), callers.end());
        return callStack;
    }

    bool CFG::isStatementTransitivelyNextBip(StatementNum id1, StatementNum id2) const
    {
        check_in_range(id1, total_inst);
        check_in_range(id2, total_inst);
        StatementSet callStack = getCurrentStack(id1);
        StatementSet visited;
        std::queue<StatementNum> q;

        auto addNextNodes = [&](StatementNum num) {
            auto callStmt = getCallStmtMapping(num);
            if(callStmt != nullptr)
            {
                callStack.insert(num);
                auto name = dynamic_cast<const simple::ast::ProcCall*>(callStmt->getAstStmt())->proc_name;
                for(auto return_pt : gates.at(name).second)
                {
                    q.emplace(return_pt);
                }
            }
            if(auto it = adj_lst_bip.find(num); it != adj_lst_bip.end())
            {
                for(auto& [stmt, weight] : it->second)
                {
                    if(weight == 1 || callStack.count(weight - 1) != 0) // intra or allowed to visit
                    {
                        if(visited.count(stmt) == 0) // not visited yet
                        {
                            q.emplace(stmt);
                        }
                    }
                }
            }
        };

        addNextNodes(id1);
        while(!q.empty())
        {
            auto curr = q.front();
            if(curr == id2)
                return true;
            addNextNodes(curr);
            visited.insert(curr);
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
        auto& stmt = m_pkb->getStatementAt(id);

        if(auto cache = stmt.maybeGetTransitivelyNextStatementsBip(); cache != nullptr)
            return *cache;

        StatementSet callStack = getCurrentStack(id);
        StatementSet visited;
        std::queue<StatementNum> q;

        auto addNextNodes = [&](StatementNum num) {
            auto callStmt = getCallStmtMapping(num);
            if(callStmt != nullptr)
            {
                callStack.insert(num);
                auto name = dynamic_cast<const simple::ast::ProcCall*>(callStmt->getAstStmt())->proc_name;
                for(auto return_pt : gates.at(name).second)
                {
                    q.emplace(return_pt);
                }
            }
            if(auto it = adj_lst_bip.find(num); it != adj_lst_bip.end())
            {
                for(auto& [stmt, weight] : it->second)
                {
                    if(weight == 1 || callStack.count(weight - 1) != 0) // intra or allowed to visit
                    {
                        if(visited.count(stmt) == 0) // not visited yet
                        {
                            q.emplace(stmt);
                        }
                    }
                }
            }
        };

        addNextNodes(id);
        while(!q.empty())
        {
            auto curr = q.front();
            addNextNodes(curr);
            visited.insert(curr);
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
            if(isStatementTransitivelyNextBip(i + 1, id))
            {
                ret.insert(i + 1);
            }
        }
        return stmt.cacheTransitivelyPreviousStatementsBip(std::move(ret));
    }
}
