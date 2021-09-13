// follows.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = util::PqlException;

    void Evaluator::handleFollows(const ast::Follows* follows)
    {
        assert(follows);
        assert(follows->directly_after);
        assert(follows->directly_before);

        bool is_bef_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_before);
        bool is_aft_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_after);
        bool is_bef_all = dynamic_cast<ast::AllStmt*>(follows->directly_before);
        bool is_aft_all = dynamic_cast<ast::AllStmt*>(follows->directly_after);
        bool is_bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_before);
        bool is_aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_after);

        if(is_bef_stmt_id && is_aft_stmt_id)
        {
            auto bef_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_before)->id;
            auto aft_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_after)->id;

            util::log("pql::eval", "Processing Follows(StmtId,StmtId)");
            if(m_pkb->isFollows(bef_stmt_id, aft_stmt_id))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
            else
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
        }
        else if(is_bef_stmt_id && is_aft_all)
        {
            auto bef_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_before)->id;

            util::log("pql::eval", "Processing Follows(StmtId,_)");
            if(m_pkb->getFollows(bef_stmt_id)->after.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
        }
        else if(is_bef_stmt_id && is_aft_decl)
        {
            auto bef_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_before)->id;
            auto aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_after)->declaration;

            util::log("pql::eval", "Processing Follows(StmtId,DeclaredStmt)");
            if(m_pkb->getFollows(bef_stmt_id)->after.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false. No statement directly after {}",
                    follows->toString(), bef_stmt_id);
            }
            else
            {
                auto entry = table::Entry(aft_decl, m_pkb->getFollows(bef_stmt_id)->directly_after);
                util::log("pql::eval", "{} updating domain to [{}]", follows->toString(), entry.toString());
                table::Domain curr_domain = { entry };
                table::Domain prev_domain = m_table->getDomain(aft_decl);
                m_table->upsertDomains(aft_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(is_bef_all && is_aft_stmt_id)
        {
            auto aft_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_after)->id;

            util::log("pql::eval", "Processing Follows(_,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id)->before.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
        }
        else if(is_bef_all && is_aft_all)
        {
            util::log("pql::eval", "Processing Follows(_,_)");
            if(m_pkb->followsRelationExists())
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
            else
            {
                throw util::PqlException(
                    "pql::eval", "{} will always evaluate to false. No 2 following statement.", follows->toString());
            }
        }
        else if(is_bef_all && is_aft_decl)
        {
            auto aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_after)->declaration;

            util::log("pql::eval", "Processing Follows(_,DeclaredStmt)");
            table::Domain curr_domain {};
            table::Domain prev_domain = m_table->getDomain(aft_decl);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->before.empty())
                {
                    table::Entry entry = table::Entry(aft_decl, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(aft_decl, table::entry_set_intersect(prev_domain, curr_domain));
        }
        else if(is_bef_decl && is_aft_stmt_id)
        {
            auto bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_before)->declaration;
            auto aft_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_after)->id;

            util::log("pql::eval", "Processing Follows(DeclaredStmt,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id)->before.empty())
            {
                throw PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly directly before {}", follows->toString(),
                    aft_stmt_id);
            }
            else
            {
                auto entry = table::Entry(bef_decl, m_pkb->getFollows(aft_stmt_id)->directly_before);
                util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                table::Domain curr_domain = { entry };
                table::Domain prev_domain = m_table->getDomain(bef_decl);
                m_table->upsertDomains(bef_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(is_bef_decl && is_aft_all)
        {
            auto bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_before)->declaration;

            util::log("pql::eval", "Processing Follows(DeclaredStmt,_)");
            table::Domain curr_domain;
            table::Domain prev_domain = m_table->getDomain(bef_decl);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->after.empty())
                {
                    table::Entry entry = table::Entry(bef_decl, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(bef_decl, table::entry_set_intersect(prev_domain, curr_domain));
        }
        else if(is_bef_decl && is_aft_decl)
        {
            auto bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_before)->declaration;
            auto aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_after)->declaration;

            util::log("pql::eval", "Processing Follows(DeclaredStmt,DeclaredStmt)");

            // use a combination of pruning and intersection in this case, to obviate the need for
            // explicitly doing a nested loop.

            auto bef_domain = m_table->getDomain(bef_decl);
            auto new_aft_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = bef_domain.begin(); it != bef_domain.end();)
            {
                auto bef_follows = m_pkb->getFollows(it->getStmtNum());
                if(bef_follows->after.empty())
                {
                    it = bef_domain.erase(it);
                    continue;
                }

                auto bef_entry = table::Entry(bef_decl, bef_follows->id);
                auto aft_entry = table::Entry(aft_decl, bef_follows->directly_after);

                new_aft_domain.insert(aft_entry);

                util::log("pql::eval", "{} adds Join({}, {})", follows->toString(), bef_entry.toString(),
                    aft_entry.toString());
                allowed_entries.insert({ bef_entry, aft_entry });
                ++it;
            }

            m_table->upsertDomains(bef_decl, bef_domain);
            m_table->upsertDomains(aft_decl, table::entry_set_intersect(new_aft_domain, m_table->getDomain(aft_decl)));
            m_table->addJoin(table::Join(bef_decl, aft_decl, allowed_entries));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable: invalid combination of argument types");
        }
    }




    void Evaluator::handleFollowsT(const ast::FollowsT* follows_t)
    {
        assert(follows_t);
        assert(follows_t->after);
        assert(follows_t->before);

        bool is_bef_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->before);
        bool is_aft_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->after);
        bool is_bef_all = dynamic_cast<ast::AllStmt*>(follows_t->before);
        bool is_aft_all = dynamic_cast<ast::AllStmt*>(follows_t->after);
        bool is_bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->before);
        bool is_aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->after);

        if(is_bef_stmt_id && is_aft_stmt_id)
        {
            auto bef_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->before)->id;
            auto aft_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->after)->id;

            util::log("pql::eval", "Processing Follows*(StmtId,StmtId)");
            if(m_pkb->isFollowsT(bef_stmt_id, aft_stmt_id))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
            else
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
        }
        else if(is_bef_stmt_id && is_aft_all)
        {
            auto bef_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->before)->id;

            util::log("pql::eval", "Processing Follows*(StmtId,_)");
            if(m_pkb->getFollows(bef_stmt_id)->after.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
        }
        else if(is_bef_stmt_id && is_aft_decl)
        {
            auto bef_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->before)->id;
            auto aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->after)->declaration;

            util::log("pql::eval", "Processing Follows*(StmtId,DeclaredStmt)");
            if(m_pkb->getFollows(bef_stmt_id)->after.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false. No statement directly after {}",
                    follows_t->toString(), bef_stmt_id);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                for(simple::ast::StatementNum after_stmt_num : m_pkb->getFollows(bef_stmt_id)->after)
                {
                    table::Entry entry = table::Entry(aft_decl, after_stmt_num);
                    util::log("pql::eval", "{} updating domain to [{}]", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl);
                m_table->upsertDomains(aft_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(is_bef_all && is_aft_stmt_id)
        {
            auto aft_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->after)->id;

            util::log("pql::eval", "Processing Follows*(_,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id)->before.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
        }
        else if(is_bef_all && is_aft_all)
        {
            util::log("pql::eval", "Processing Follows*(_,_)");
            if(m_pkb->followsRelationExists())
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
            else
            {
                throw util::PqlException(
                    "pql::eval", "{} will always evaluate to false. No 2 following statement.", follows_t->toString());
            }
        }
        else if(is_bef_all && is_aft_decl)
        {
            auto aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->after)->declaration;

            util::log("pql::eval", "Processing Follows*(_,DeclaredStmt)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->before.empty())
                {
                    table::Entry entry = table::Entry(aft_decl, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(aft_decl, table::entry_set_intersect(prev_domain, curr_domain));
        }
        else if(is_bef_decl && is_aft_stmt_id)
        {
            auto aft_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->after)->id;
            auto bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->before)->declaration;

            util::log("pql::eval", "Processing Follows*(DeclaredStmt,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id)->before.empty())
            {
                throw PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly directly before {}", follows_t->toString(),
                    aft_stmt_id);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl);
                for(simple::ast::StatementNum bef_stmt_id : m_pkb->getFollows(aft_stmt_id)->before)
                {
                    auto entry = table::Entry(bef_decl, bef_stmt_id);
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                }
                m_table->upsertDomains(bef_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(is_bef_decl && is_aft_all)
        {
            auto bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->before)->declaration;

            util::log("pql::eval", "Processing Follows*(DeclaredStmt,_)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->after.empty())
                {
                    table::Entry entry = table::Entry(bef_decl, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(bef_decl, table::entry_set_intersect(prev_domain, curr_domain));
        }
        else if(is_bef_decl && is_aft_decl)
        {
            auto bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->before)->declaration;
            auto aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->after)->declaration;

            util::log("pql::eval", "Processing Follows*(DeclaredStmt,DeclaredStmt)");

            // same strategy as Follows
            auto bef_domain = m_table->getDomain(bef_decl);
            auto new_aft_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = bef_domain.begin(); it != bef_domain.end();)
            {
                auto bef_follows = m_pkb->getFollows(it->getStmtNum());
                if(bef_follows->after.empty())
                {
                    it = bef_domain.erase(it);
                    continue;
                }

                for(auto aft_stmt_num : bef_follows->after)
                {
                    auto bef_entry = table::Entry(bef_decl, bef_follows->id);
                    auto aft_entry = table::Entry(aft_decl, aft_stmt_num);

                    util::log("pql::eval", "{} adds Join({}, {})", follows_t->toString(), bef_entry.toString(),
                        aft_entry.toString());

                    new_aft_domain.insert(aft_entry);
                    allowed_entries.insert({ bef_entry, aft_entry });
                }
                ++it;
            }

            m_table->upsertDomains(bef_decl, bef_domain);
            m_table->upsertDomains(aft_decl, table::entry_set_intersect(new_aft_domain, m_table->getDomain(aft_decl)));
            m_table->addJoin(table::Join(bef_decl, aft_decl, allowed_entries));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable: invalid combination of argument types");
        }
    }
}
