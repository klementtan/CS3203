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
        auto* bef_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_before);
        auto* aft_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_after);
        auto* bef_all = dynamic_cast<ast::AllStmt*>(follows->directly_before);
        auto* aft_all = dynamic_cast<ast::AllStmt*>(follows->directly_after);
        auto* bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_before);
        auto* aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_after);

        if(bef_stmt_id && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows(StmtId,StmtId)");
            if(m_pkb->isFollows(bef_stmt_id->id, aft_stmt_id->id))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
            else
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
        }
        if(bef_stmt_id && aft_all)
        {
            util::log("pql::eval", "Processing Follows(StmtId,_)");
            if(m_pkb->getFollows(bef_stmt_id->id)->after.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
        }
        if(bef_stmt_id && aft_decl)
        {
            util::log("pql::eval", "Processing Follows(StmtId,DeclaredStmt)");
            if(m_pkb->getFollows(bef_stmt_id->id)->after.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false. No statement directly after {}",
                    follows->toString(), bef_stmt_id->toString());
            }
            else
            {
                table::Entry entry =
                    table::Entry(aft_decl->declaration, m_pkb->getFollows(bef_stmt_id->id)->directly_after);
                util::log("pql::eval", "{} updating domain to [{}]", follows->toString(), entry.toString());
                std::unordered_set<table::Entry> curr_domain = { entry };
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl->declaration);
                m_table->upsertDomains(aft_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(bef_all && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows(_,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id->id)->before.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
        }
        if(bef_all && aft_all)
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
        if(bef_all && aft_decl)
        {
            util::log("pql::eval", "Processing Follows(_,DeclaredStmt)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl->declaration);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->before.empty())
                {
                    table::Entry entry = table::Entry(aft_decl->declaration, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(aft_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(bef_decl && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows(DeclaredStmt,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id->id)->before.empty())
            {
                throw PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly directly before {}", follows->toString(),
                    aft_stmt_id->toString());
            }
            else
            {
                auto entry = table::Entry(bef_decl->declaration, m_pkb->getFollows(aft_stmt_id->id)->directly_before);
                util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                std::unordered_set<table::Entry> curr_domain = { entry };
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl->declaration);
                m_table->upsertDomains(bef_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(bef_decl && aft_all)
        {
            util::log("pql::eval", "Processing Follows(DeclaredStmt,_)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl->declaration);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->after.empty())
                {
                    table::Entry entry = table::Entry(bef_decl->declaration, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(bef_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(bef_decl && aft_decl)
        {
            util::log("pql::eval", "Processing Follows(DeclaredStmt,DeclaredStmt)");
            for(const table::Entry& entry : m_table->getDomain(bef_decl->declaration))
            {
                pkb::Follows* bef_follows = m_pkb->getFollows(entry.getStmtNum());
                if(bef_follows->after.empty())
                    continue;
                table::Entry bef_entry = table::Entry(bef_decl->declaration, bef_follows->id);
                table::Entry aft_entry = table::Entry(aft_decl->declaration, bef_follows->directly_after);
                util::log("pql::eval", "{} adds Join({},{})", follows->toString(), bef_entry.toString(),
                    aft_entry.toString());
                m_table->addJoin(bef_entry, aft_entry);
            }
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
            for(const table::Entry& entry : m_table->getDomain(bef_decl->declaration))
            {
                pkb::Follows* bef_follows = m_pkb->getFollows(entry.getStmtNum());
                if(bef_follows->after.empty())
                    continue;
                for(simple::ast::StatementNum after_stmt_num : bef_follows->after)
                {
                    table::Entry bef_entry = table::Entry(bef_decl->declaration, bef_follows->id);
                    table::Entry aft_entry = table::Entry(aft_decl->declaration, after_stmt_num);
                    util::log("pql::eval", "{} adds Join({},{})", follows_t->toString(), bef_entry.toString(),
                        aft_entry.toString());
                    m_table->addJoin(bef_entry, aft_entry);
                }
            }
        }
    }
}
