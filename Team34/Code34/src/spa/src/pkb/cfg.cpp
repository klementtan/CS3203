// cfg.cpp

#include "pkb.h"
#include "exceptions.h"

#include <zpr.h>
#include <assert.h>

#define INF SIZE_MAX

namespace pkb
{
    using StatementNum = simple::ast::StatementNum;

    CFG::CFG(size_t v)
    {
        total_inst = v;
        adj_mat = new size_t*[v];
        m_next_exists = false;

        for(size_t i = 0; i < v; i++)
        {
            this->adj_mat[i] = new size_t[v];
            for(size_t j = 0; j < v; j++)
            {
                adj_mat[i][j] = INF;
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
    }

    bool CFG::nextRelationExists() const
    {
        return m_next_exists;
    }

    std::string CFG::getMatRep() const
    {
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
                res += zpr::sprint("{03} ", adj_mat[i][j] == INF ? 0 : adj_mat[i][j]);
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

    bool CFG::isStatementNext(StatementNum stmt1, StatementNum stmt2) const
    {
        if(stmt1 > total_inst || stmt1 <= 0 || stmt2 > total_inst || stmt2 <= 0)
            throw util::PkbException("pkb", "Statement number out of range");
        return adj_mat[stmt1 - 1][stmt2 - 1] == 1;
    }

    bool CFG::isStatementTransitivelyNext(StatementNum stmt1, StatementNum stmt2) const
    {
        if(stmt1 > total_inst || stmt1 <= 0 || stmt2 > total_inst || stmt2 <= 0)
            throw util::PkbException("pkb", "Statement number out of range");
        return adj_mat[stmt1 - 1][stmt2 - 1] < INF; // impossible to be 0 since no recursive call
    }
    StatementSet CFG::getNextStatements(StatementNum id) const
    {
        if(id > total_inst || id <= 0)
            throw util::PkbException("pkb", "Statement number out of range");
        StatementSet ret {};
        for(size_t j = 0; j < total_inst; j++)
        {
            if(adj_mat[id - 1][j] == 1)
                ret.insert(j + 1);
        }
        return ret;
    }
    StatementSet CFG::getTransitivelyNextStatements(StatementNum id) const
    {
        if(id > total_inst || id <= 0)
            throw util::PkbException("pkb", "Statement number out of range");
        StatementSet ret {};
        for(size_t j = 0; j < total_inst; j++)
        {
            if(adj_mat[id - 1][j] < INF)
                ret.insert(j + 1);
        }
        return ret;
    }

}
