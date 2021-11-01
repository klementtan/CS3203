// cfg.cpp

#include "pkb.h"
#include "exceptions.h"

#include <zpr.h>
#include <assert.h>
#include <queue>
#include <unordered_set>
#include <iostream>
#include <utility>
#include <algorithm>
#include <vector>

#define INF SIZE_MAX

namespace pkb
{
    using StatementNum = simple::ast::StatementNum;

    CFG::CFG(size_t v)
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
            delete[] this->adj_mat[i];

        delete[] this->adj_mat;
    }

    void CFG::addEdge(StatementNum stmt1, StatementNum stmt2)
    {
        assert(stmt1 <= total_inst && stmt1 > 0);
        assert(stmt2 <= total_inst && stmt2 > 0);
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

    void CFG::addEdgeBip(StatementNum stmt1, StatementNum stmt2, size_t weight)
    {
        assert(stmt1 <= total_inst && stmt1 > 0);
        assert(stmt2 <= total_inst && stmt2 > 0);
        adj_mat_bip[stmt1 - 1][stmt2 - 1] = weight;
        if(weight == INF)
        {
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
                if(adj_lst_bip[stmt1].size()==0)
                {
                    adj_lst_bip.erase(stmt1);
                }
            }
        }
        else
        {
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
                for(int a = 1; a < total_inst+1; a++)
                {
                    if(adj_lst_bip.count(a) > 0)
                    {
                        std::cout << "at " << a << std::endl;

                        for(auto& i : adj_lst_bip.at(a))
                        {
                            std::cout << i.first << " " << i.second << std::endl;
                        }
                    }

                }

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
                res += zpr::sprint("{03} ", mat[i][j] == INF ? 0 : mat[i][j]);
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

    static void check_in_range(StatementNum num, size_t max)
    {
        if(num > max || num <= 0)
            throw util::PkbException("pkb", "Statement number out of range");
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
        if(assign_stmts.count(id) == 0)
            return nullptr;
        return assign_stmts.at(id);
    }

    const Statement* CFG::getCallStmtMapping(StatementNum id) const
    {
        if(call_stmts.count(id) == 0)
            return nullptr;
        return call_stmts.at(id);
    }

    const Statement* CFG::getModStmtMapping(StatementNum id) const
    {
        if(mod_stmts.count(id) == 0)
            return nullptr;
        return mod_stmts.at(id);
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

    StatementSet CFG::getNextStatements(StatementNum id) const
    {
        check_in_range(id, total_inst);

        StatementSet ret {};
        if(!adj_lst.count(id))
            return ret;

        return adj_lst.at(id);
    }

    StatementSet CFG::getTransitivelyNextStatements(StatementNum id) const
    {
        check_in_range(id, total_inst);

        StatementSet ret {};
        for(size_t j = 0; j < total_inst; j++)
        {
            if(adj_mat[id - 1][j] < INF)
                ret.insert(j + 1);
        }
        return ret;
    }

    StatementSet CFG::getPreviousStatements(StatementNum id) const
    {
        check_in_range(id, total_inst);
        StatementSet ret {};

        for(size_t i = 0; i < this->total_inst; i++)
        {
            if(adj_mat[i][id - 1] == 1)
                ret.insert(i + 1);
        }

        return ret;
    }

    StatementSet CFG::getTransitivelyPreviousStatements(StatementNum id) const
    {
        check_in_range(id, total_inst);
        StatementSet ret {};

        for(size_t i = 0; i < this->total_inst; i++)
        {
            if(adj_mat[i][id - 1] < INF)
                ret.insert(i + 1);
        }

        return ret;
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

            for(auto stmt : adj_lst.at(num))
            {
                if(visited.count(stmt))
                    continue;
                visited.insert(stmt);
                q.emplace(stmt, var);
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

    StatementSet CFG::getAffectedStatements(StatementNum id) const
    {
        StatementSet ret {};
        for(auto stmt : getTransitivelyNextStatements(id))
            if(doesAffect(id, stmt))
                ret.insert(stmt);
        return ret;
    }

    StatementSet CFG::getAffectingStatements(StatementNum id) const
    {
        StatementSet ret {};
        for(auto stmt : getTransitivelyPreviousStatements(id))
            if(doesAffect(stmt, id))
                ret.insert(stmt);
        return ret;
    }

    StatementSet CFG::getTransitivelyAffectedStatements(StatementNum id) const
    {
        StatementSet ret {};
        for(auto stmt : getTransitivelyNextStatements(id))
            if(doesTransitivelyAffect(id, stmt))
                ret.insert(stmt);
        return ret;
    }

    StatementSet CFG::getTransitivelyAffectingStatements(StatementNum id) const
    {
        StatementSet ret {};
        for(auto stmt : getTransitivelyPreviousStatements(id))
            if(doesTransitivelyAffect(stmt, id))
                ret.insert(stmt);
        return ret;
    }

    bool CFG::doesAffectBip(StatementNum id1, StatementNum id2) const {
        auto stmt1 = getAssignStmtMapping(id1);
        auto stmt2 = getAssignStmtMapping(id2);

        if(stmt1 == nullptr || stmt2 == nullptr)
            return false;

        const auto& modified = stmt1->getModifiedVariables();
        const auto& used = stmt2->getUsedVariables();
        assert(modified.size() == 1);
        const auto var = modified.begin();
        if(used.count(*var) == 0)
        {
            return false;
        }
        std::vector<StatementNum> call_stack {};
        StatementSet visited;

        //getAssignStmtMapping(id1)->getAstStmt()
    }
}
