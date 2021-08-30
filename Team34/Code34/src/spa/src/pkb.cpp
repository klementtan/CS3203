#include <cassert>

#include <zpr.h>

#include "ast.h"
#include "pkb.h"
#include "util.h"

namespace pkb
{
    ProgramKB* pkb;

    // collection only entails numbering the statements
    static void collectStmtList(ast::StmtList* list);
    static void collectStmt(ast::Stmt* stmt, ast::StmtList* parent)
    {
        // the statement should not have been seen yet.
        assert(stmt->id == 0);

        stmt->parent_list = parent;
        stmt->id = pkb->statements.size() + 1;
        pkb->statements.push_back(stmt);

        if(auto i = dynamic_cast<ast::IfStmt*>(stmt); i)
        {
            pkb->if_statements.push_back(stmt);
            collectStmtList(&i->true_case);
            collectStmtList(&i->false_case);
            i->true_case.parent_statement = stmt;
            i->false_case.parent_statement = stmt;
        }
        else if(auto w = dynamic_cast<ast::WhileLoop*>(stmt); w)
        {
            pkb->while_statements.push_back(stmt);
            collectStmtList(&w->body);
            w->body.parent_statement = stmt;
        }
    }

    static void collectStmtList(ast::StmtList* list)
    {
        for(const auto& stmt : list->statements)
            collectStmt(stmt, list);
    }

    // processes and populates Follows and FollowsT concurrently
    static void processFollows(ast::StmtList* list)
    {
        std::vector<ast::Stmt*> stmt_list = list->statements;
        for(size_t i = 0; i < list->statements.size(); i++)
        {
            ast::Stmt* stmt = stmt_list[i];

            auto follows = new Follows();
            follows->id = stmt->id;
            pkb->follows.push_back(follows);

            for(size_t j = i + 1; j < list->statements.size(); j++)
            {
                if(j - i == 1)
                {
                    follows->directly_after = stmt_list[j]->id;
                }
                follows->after.push_back(stmt_list[j]->id);
            }

            for(size_t k = 0; k < i; k++)
            {
                if(i - k == 1)
                {
                    follows->directly_before = stmt_list[k]->id;
                }
                follows->before.push_back(stmt_list[k]->id);
            }

            if(auto i = dynamic_cast<ast::IfStmt*>(stmt); i)
            {
                processFollows(&i->true_case);
                processFollows(&i->false_case);
            }
            else if(auto w = dynamic_cast<ast::WhileLoop*>(stmt); w)
            {
                processFollows(&w->body);
            }
        }
    }

    // Takes in two 1-indexed StatementNums
    bool isFollows(ast::StatementNum fst, ast::StatementNum snd)
    {
        // thinking of more elegant ways of handling this hmm
        if(fst > pkb->follows.size() || snd > pkb->follows.size() || fst < 1 || snd < 1)
            util::error("pkb", "StatementNum out of range.");

        return pkb->follows[fst - 1]->directly_after == snd;
    }

    // Same as isFollows
    bool isFollowsT(ast::StatementNum fst, ast::StatementNum snd)
    {
        if(fst > pkb->follows.size() || snd > pkb->follows.size() || fst < 1 || snd < 1)
            util::error("pkb", "StatementNum out of range.");

        auto& after_list = pkb->follows[fst - 1]->after;
        return std::find(after_list.begin(), after_list.end(), snd) != after_list.end();
    }

    // Takes in two 1-indexed StatementNums. Allows for 0 to be used as a wildcard on 1 of the parameters.
    std::vector<ast::StatementNum> getFollowsTList(ast::StatementNum fst, ast::StatementNum snd)
    {
        if(fst > pkb->follows.size() || snd > pkb->follows.size() || fst < 0 || snd < 0)
            util::error("pkb", "StatementNum out of range.");
        if(fst < 1 && snd < 1 || fst != 0 && snd != 0)
            util::error("pkb", "Only 1 wildcard is to be used.");

        if(fst == 0)
        {
            return pkb->follows[snd - 1]->before;
        }
        else if(snd == 0)
        {
            return pkb->follows[fst - 1]->after;
        }

        // Should not be reachable.
        util::error("pkb", "Unexpected error.");
    }

    ProgramKB* processProgram(ast::Program* program)
    {
        pkb = new ProgramKB();

        // do a first pass to number all the statements, set the
        // parent stmtlist, and collect all the procedures.
        for(const auto& proc : program->procedures)
        {
            collectStmtList(&proc->body);

            if(pkb->procedures.find(proc->name) != pkb->procedures.end())
                util::error("pkb", "procedure '{}' is already defined", proc->name);

            pkb->procedures[proc->name].ast_proc = proc;
        }

        // do a second pass to populate the follows vector.
        for(const auto& proc : program->procedures)
        {
            processFollows(&proc->body);
        }

        return pkb;
    }
}
