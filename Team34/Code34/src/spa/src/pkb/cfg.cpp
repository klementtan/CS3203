// cfg.cpp

#include "pkb.h"

#include <zpr.h>
#include <assert.h>

#define INF INT_MAX

namespace pkb
{
    CFG::CFG(int v)
    {
        total_inst = v;
        adj_mat = new int*[v];
        for(int i = 0; i < v; i++)
        {
            this->adj_mat[i] = new int[v];
            for(int j = 0; j < v; j++)
            {
                adj_mat[i][j] = INF;
            }
        }
    }

    void CFG::addEdge(int stmt_no1, int stmt_no2)
    {
        assert(stmt_no1 <= total_inst && stmt_no1 > 0);
        assert(stmt_no2 <= total_inst && stmt_no2 > 0);
        adj_mat[stmt_no1 - 1][stmt_no2 - 1] = 1;
    }

    void CFG::showMat()
    {
        zpr::print("printing matrix with side {}\n", total_inst);
        zpr::print("      ");
        for(int i = 0; i < total_inst; i++)
        {
            zpr::print("{03} ", i + 1); 
        }
        zpr::print("\n");

        for(int i = 0; i < total_inst; i++)
        {
            zpr::print("{03} | ", i+1);
            for(int j = 0; j <total_inst; j++)
            {
                zpr::print("{03} ", adj_mat[i][j] == INF ? -1 : adj_mat[i][j]);
            }
            zpr::print("\n");
        }
    }

    void CFG::computeDistMat()
    {
        // Adapted from https://www.geeksforgeeks.org/floyd-warshall-algorithm-dp-16/
        int k, i, j;

        for(k = 0; k < total_inst; k++)
        {
            for(i = 0; i < total_inst; i++)
            {
                for(j = 0; j < total_inst; j++)
                {
                    if(adj_mat[i][j] > (adj_mat[i][k] + adj_mat[k][j]) &&
                        (adj_mat[k][j] != INF && adj_mat[i][k] != INF))
                        adj_mat[i][j] = adj_mat[i][k] + adj_mat[k][j];
                }
            }
        }
    }
}
