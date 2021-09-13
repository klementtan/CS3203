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
            table::Domain aft_domain {};

            for(auto it = bef_domain.begin(); it != bef_domain.end();)
            {
                auto bef_follows = m_pkb->getFollows(it->getStmtNum());
                if(bef_follows->after.empty())
                {
                    it = bef_domain.erase(it);
                }
                else
                {
                    auto bef_entry = table::Entry(bef_decl, bef_follows->id);
                    auto aft_entry = table::Entry(aft_decl, bef_follows->directly_after);

                    aft_domain.insert(aft_entry);

                    util::log("pql::eval", "{} adds Join({}, {})", follows->toString(), bef_entry.toString(),
                        aft_entry.toString());

                    m_table->addJoin(bef_entry, aft_entry);
                    ++it;
                }
            }

            m_table->upsertDomains(bef_decl, bef_domain);
            m_table->upsertDomains(aft_decl, table::entry_set_intersect(aft_domain, m_table->getDomain(aft_decl)));
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
        auto* bef_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->before);
        auto* aft_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->after);
        auto* bef_all = dynamic_cast<ast::AllStmt*>(follows_t->before);
        auto* aft_all = dynamic_cast<ast::AllStmt*>(follows_t->after);
        auto* bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->before);
        auto* aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->after);

        if(bef_stmt_id && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows*(StmtId,StmtId)");
            if(m_pkb->isFollowsT(bef_stmt_id->id, aft_stmt_id->id))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
            else
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
        }
        if(bef_stmt_id && aft_all)
        {
            util::log("pql::eval", "Processing Follows*(StmtId,_)");
            if(m_pkb->getFollows(bef_stmt_id->id)->after.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
        }
        if(bef_stmt_id && aft_decl)
        {
            util::log("pql::eval", "Processing Follows*(StmtId,DeclaredStmt)");
            if(m_pkb->getFollows(bef_stmt_id->id)->after.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false. No statement directly after {}",
                    follows_t->toString(), bef_stmt_id->toString());
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                for(simple::ast::StatementNum after_stmt_num : m_pkb->getFollows(bef_stmt_id->id)->after)
                {
                    table::Entry entry = table::Entry(aft_decl->declaration, after_stmt_num);
                    util::log("pql::eval", "{} updating domain to [{}]", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl->declaration);
                m_table->upsertDomains(aft_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(bef_all && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows*(_,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id->id)->before.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
        }
        if(bef_all && aft_all)
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
        if(bef_all && aft_decl)
        {
            util::log("pql::eval", "Processing Follows*(_,DeclaredStmt)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl->declaration);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->before.empty())
                {
                    table::Entry entry = table::Entry(aft_decl->declaration, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(aft_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(bef_decl && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows*(DeclaredStmt,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id->id)->before.empty())
            {
                throw PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly directly before {}", follows_t->toString(),
                    aft_stmt_id->toString());
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl->declaration);
                for(simple::ast::StatementNum bef_stmt_id : m_pkb->getFollows(aft_stmt_id->id)->before)
                {
                    auto entry = table::Entry(bef_decl->declaration, bef_stmt_id);
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                }
                m_table->upsertDomains(bef_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(bef_decl && aft_all)
        {
            util::log("pql::eval", "Processing Follows*(DeclaredStmt,_)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl->declaration);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->after.empty())
                {
                    table::Entry entry = table::Entry(bef_decl->declaration, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(bef_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(bef_decl && aft_decl)
        {
            util::log("pql::eval", "Processing Follows*(DeclaredStmt,DeclaredStmt)");

            auto old_domain = m_table->getDomain(bef_decl->declaration);
            for(auto it = old_domain.begin(); it != old_domain.end();)
            {
                auto bef_follows = m_pkb->getFollows(it->getStmtNum());
                if(bef_follows->after.empty())
                {
                    it = old_domain.erase(it);
                }
                else
                {
                    for(auto aft_stmt_num : bef_follows->after)
                    {
                        auto bef_entry = table::Entry(bef_decl->declaration, bef_follows->id);
                        auto aft_entry = table::Entry(aft_decl->declaration, aft_stmt_num);
                        util::log("pql::eval", "{} adds Join({}, {})", follows_t->toString(), bef_entry.toString(),
                            aft_entry.toString());

                        m_table->addJoin(bef_entry, aft_entry);
                    }
                    ++it;
                }
            }
        }
    }
}
