// call_graph.cpp

#include "pkb.h"

namespace pkb
{
    namespace s_ast = simple::ast;

    void CallGraph::addEdge(const std::string& a, std::string b)
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

    std::string CallGraph::missingProc(const std::vector<std::unique_ptr<s_ast::Procedure>>& procs)
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
}
