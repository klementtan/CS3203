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

        const auto& before_stmt = follows->directly_before;
        const auto& after_stmt = follows->directly_after;

        if(before_stmt.isDeclaration())
            m_table.addSelectDecl(before_stmt.declaration());

        if(after_stmt.isDeclaration())
            m_table.addSelectDecl(after_stmt.declaration());

        if(before_stmt.isStatementId() && after_stmt.isStatementId())
        {
            auto bef_stmt_id = before_stmt.id();
            auto aft_stmt_id = after_stmt.id();

            util::log("pql::eval", "Processing Follows(StmtId,StmtId)");
            // if(m_pkb->isFollows(bef_stmt_id, aft_stmt_id))
            if(m_pkb->getStatementAtIndex(aft_stmt_id)->follows(bef_stmt_id))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
            else
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
        }
        else if(before_stmt.isStatementId() && after_stmt.isWildcard())
        {
            auto bef_stmt_id = before_stmt.id();

            util::log("pql::eval", "Processing Follows(StmtId,_)");
            // if(m_pkb->getFollows(bef_stmt_id)->after.empty())
            if(!m_pkb->getStatementAtIndex(bef_stmt_id)->hasFollower())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
        }
        else if(before_stmt.isStatementId() && after_stmt.isDeclaration())
        {
            auto bef_stmt_id = before_stmt.id();
            auto aft_decl = after_stmt.declaration();

            auto bef_stmt = m_pkb->getStatementAtIndex(bef_stmt_id);

            util::log("pql::eval", "Processing Follows(StmtId,DeclaredStmt)");
            // if(m_pkb->getFollows(bef_stmt_id)->after.empty())
            if(!bef_stmt->hasFollower())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false. No statement directly after {}",
                    follows->toString(), bef_stmt_id);
            }
            else
            {
                auto entry = table::Entry(aft_decl, bef_stmt->getDirectFollower());
                util::log("pql::eval", "{} updating domain to [{}]", follows->toString(), entry.toString());
                table::Domain curr_domain = { entry };
                table::Domain prev_domain = m_table.getDomain(aft_decl);
                m_table.upsertDomains(aft_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(before_stmt.isWildcard() && after_stmt.isStatementId())
        {
            auto aft_stmt_id = after_stmt.id();

            util::log("pql::eval", "Processing Follows(_,StmtId)");
            // if(m_pkb->getFollows(aft_stmt_id)->before.empty())
            if(!m_pkb->getStatementAtIndex(aft_stmt_id)->isFollower())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
        }
        else if(before_stmt.isWildcard() && after_stmt.isWildcard())
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
        else if(before_stmt.isWildcard() && after_stmt.isDeclaration())
        {
            auto aft_decl = after_stmt.declaration();

            util::log("pql::eval", "Processing Follows(_,DeclaredStmt)");

            auto domain = m_table.getDomain(aft_decl);
            for(auto it = domain.begin(); it != domain.end();)
            {
                if(!m_pkb->getStatementAtIndex(it->getStmtNum())->isFollower())
                    it = domain.erase(it);
                else
                    ++it;
            }

            m_table.upsertDomains(aft_decl, domain);
        }
        else if(before_stmt.isDeclaration() && after_stmt.isStatementId())
        {
            auto bef_decl = before_stmt.declaration();
            auto aft_stmt_id = after_stmt.id();

            auto aft_stmt = m_pkb->getStatementAtIndex(aft_stmt_id);

            util::log("pql::eval", "Processing Follows(DeclaredStmt,StmtId)");
            // if(m_pkb->getFollows(aft_stmt_id)->before.empty())
            if(!aft_stmt->isFollower())
            {
                throw PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly directly before {}", follows->toString(),
                    aft_stmt_id);
            }
            else
            {
                auto entry = table::Entry(bef_decl, aft_stmt->getDirectFollowee());
                util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                table::Domain curr_domain = { entry };
                table::Domain prev_domain = m_table.getDomain(bef_decl);
                m_table.upsertDomains(bef_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(before_stmt.isDeclaration() && after_stmt.isWildcard())
        {
            auto bef_decl = before_stmt.declaration();

            util::log("pql::eval", "Processing Follows(DeclaredStmt,_)");

            auto domain = m_table.getDomain(bef_decl);
            for(auto it = domain.begin(); it != domain.end();)
            {
                if(!m_pkb->getStatementAtIndex(it->getStmtNum())->hasFollower())
                    it = domain.erase(it);
                else
                    ++it;
            }

            m_table.upsertDomains(bef_decl, domain);
        }
        else if(before_stmt.isDeclaration() && after_stmt.isDeclaration())
        {
            auto bef_decl = before_stmt.declaration();
            auto aft_decl = after_stmt.declaration();

            util::log("pql::eval", "Processing Follows(DeclaredStmt,DeclaredStmt)");

            // use a combination of pruning and intersection in this case, to obviate the need for
            // explicitly doing a nested loop.

            auto bef_domain = m_table.getDomain(bef_decl);
            auto new_aft_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = bef_domain.begin(); it != bef_domain.end();)
            {
                auto bef_stmt = m_pkb->getStatementAtIndex(it->getStmtNum());
                if(!bef_stmt->hasFollower())
                {
                    it = bef_domain.erase(it);
                    continue;
                }

                auto bef_entry = table::Entry(bef_decl, it->getStmtNum());
                auto aft_entry = table::Entry(aft_decl, bef_stmt->getDirectFollower());

                new_aft_domain.insert(aft_entry);

                util::log("pql::eval", "{} adds Join({}, {})", follows->toString(), bef_entry.toString(),
                    aft_entry.toString());
                allowed_entries.insert({ bef_entry, aft_entry });
                ++it;
            }

            m_table.upsertDomains(bef_decl, bef_domain);
            m_table.upsertDomains(aft_decl, table::entry_set_intersect(new_aft_domain, m_table.getDomain(aft_decl)));
            m_table.addJoin(table::Join(bef_decl, aft_decl, allowed_entries));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable: invalid combination of argument types");
        }
    }




    void Evaluator::handleFollowsT(const ast::FollowsT* follows_t)
    {
        assert(follows_t);

        const auto& before_stmt = follows_t->before;
        const auto& after_stmt = follows_t->after;

        if(before_stmt.isDeclaration())
            m_table.addSelectDecl(before_stmt.declaration());

        if(after_stmt.isDeclaration())
            m_table.addSelectDecl(after_stmt.declaration());

        if(before_stmt.isStatementId() && after_stmt.isStatementId())
        {
            auto bef_stmt_id = before_stmt.id();
            auto aft_stmt_id = after_stmt.id();

            util::log("pql::eval", "Processing Follows*(StmtId,StmtId)");
            // if(m_pkb->isFollowsT(bef_stmt_id, aft_stmt_id))
            if(m_pkb->getStatementAtIndex(aft_stmt_id)->followsTransitively(bef_stmt_id))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
            else
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
        }
        else if(before_stmt.isStatementId() && after_stmt.isWildcard())
        {
            auto bef_stmt_id = before_stmt.id();

            util::log("pql::eval", "Processing Follows*(StmtId,_)");
            // if(m_pkb->getFollows(bef_stmt_id)->after.empty())
            if(!m_pkb->getStatementAtIndex(bef_stmt_id)->hasFollower())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
        }
        else if(before_stmt.isStatementId() && after_stmt.isDeclaration())
        {
            auto bef_stmt_id = before_stmt.id();
            auto aft_decl = after_stmt.declaration();

            auto bef_stmt = m_pkb->getStatementAtIndex(bef_stmt_id);

            util::log("pql::eval", "Processing Follows*(StmtId,DeclaredStmt)");
            // if(m_pkb->getFollows(bef_stmt_id)->after.empty())
            if(!bef_stmt->hasFollower())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false. No statement directly after {}",
                    follows_t->toString(), bef_stmt_id);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                for(auto after_stmt_num : bef_stmt->getTransitiveFollowers())
                {
                    table::Entry entry = table::Entry(aft_decl, after_stmt_num);
                    util::log("pql::eval", "{} updating domain to [{}]", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
                std::unordered_set<table::Entry> prev_domain = m_table.getDomain(aft_decl);
                m_table.upsertDomains(aft_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(before_stmt.isWildcard() && after_stmt.isStatementId())
        {
            auto aft_stmt_id = after_stmt.id();

            util::log("pql::eval", "Processing Follows*(_,StmtId)");
            // if(m_pkb->getFollows(aft_stmt_id)->before.empty())
            if(!m_pkb->getStatementAtIndex(aft_stmt_id)->isFollower())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
        }
        else if(before_stmt.isWildcard() && after_stmt.isWildcard())
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
        else if(before_stmt.isWildcard() && after_stmt.isDeclaration())
        {
            auto aft_decl = after_stmt.declaration();

            util::log("pql::eval", "Processing Follows*(_,DeclaredStmt)");

            auto domain = m_table.getDomain(aft_decl);
            for(auto it = domain.begin(); it != domain.end();)
            {
                if(!m_pkb->getStatementAtIndex(it->getStmtNum())->isFollower())
                    it = domain.erase(it);
                else
                    ++it;
            }

            m_table.upsertDomains(aft_decl, domain);
        }
        else if(before_stmt.isDeclaration() && after_stmt.isStatementId())
        {
            auto aft_stmt_id = after_stmt.id();
            auto bef_decl = before_stmt.declaration();

            auto aft_stmt = m_pkb->getStatementAtIndex(aft_stmt_id);

            util::log("pql::eval", "Processing Follows*(DeclaredStmt,StmtId)");
            // if(m_pkb->getFollows(aft_stmt_id)->before.empty())
            if(!aft_stmt->isFollower())
            {
                throw PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly directly before {}", follows_t->toString(),
                    aft_stmt_id);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table.getDomain(bef_decl);
                for(auto bef_stmt_id : aft_stmt->getTransitiveFollowees())
                {
                    auto entry = table::Entry(bef_decl, bef_stmt_id);
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                }
                m_table.upsertDomains(bef_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(before_stmt.isDeclaration() && after_stmt.isWildcard())
        {
            auto bef_decl = before_stmt.declaration();

            util::log("pql::eval", "Processing Follows*(DeclaredStmt,_)");

            auto domain = m_table.getDomain(bef_decl);
            for(auto it = domain.begin(); it != domain.end();)
            {
                if(!m_pkb->getStatementAtIndex(it->getStmtNum())->hasFollower())
                    it = domain.erase(it);
                else
                    ++it;
            }

            m_table.upsertDomains(bef_decl, domain);
        }
        else if(before_stmt.isDeclaration() && after_stmt.isDeclaration())
        {
            auto bef_decl = before_stmt.declaration();
            auto aft_decl = after_stmt.declaration();

            util::log("pql::eval", "Processing Follows*(DeclaredStmt,DeclaredStmt)");

            // same strategy as Follows
            auto bef_domain = m_table.getDomain(bef_decl);
            auto new_aft_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = bef_domain.begin(); it != bef_domain.end();)
            {
                auto bef_stmt = m_pkb->getStatementAtIndex(it->getStmtNum());
                if(!bef_stmt->hasFollower())
                {
                    it = bef_domain.erase(it);
                    continue;
                }

                for(auto aft_stmt_num : bef_stmt->getTransitiveFollowers())
                {
                    auto bef_entry = table::Entry(bef_decl, it->getStmtNum());
                    auto aft_entry = table::Entry(aft_decl, aft_stmt_num);

                    util::log("pql::eval", "{} adds Join({}, {})", follows_t->toString(), bef_entry.toString(),
                        aft_entry.toString());

                    new_aft_domain.insert(aft_entry);
                    allowed_entries.insert({ bef_entry, aft_entry });
                }
                ++it;
            }

            m_table.upsertDomains(bef_decl, bef_domain);
            m_table.upsertDomains(aft_decl, table::entry_set_intersect(new_aft_domain, m_table.getDomain(aft_decl)));
            m_table.addJoin(table::Join(bef_decl, aft_decl, allowed_entries));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable: invalid combination of argument types");
        }
    }
}
