// parent.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = util::PqlException;

    void Evaluator::handleParent(const ast::Parent* rel)
    {
        assert(rel);

        const auto& parent_stmt = rel->parent;
        const auto& child_stmt = rel->child;

        if(parent_stmt.isDeclaration())
            m_table.addSelectDecl(parent_stmt.declaration());
        if(child_stmt.isDeclaration())
            m_table.addSelectDecl(child_stmt.declaration());

        if(parent_stmt.isStatementId() && child_stmt.isStatementId())
        {
            util::log("pql::eval", "processing Parent(StmtId, StmtId)");

            auto parent_sid = parent_stmt.id();
            auto child_sid = child_stmt.id();

            if(!m_pkb->isParent(parent_sid, child_sid))
                throw PqlException("pql::eval", "{} is always false", rel->toString());
        }
        else if(parent_stmt.isStatementId() && child_stmt.isDeclaration())
        {
            util::log("pql::eval", "processing Parent(StmtId, DeclaredStmt)");

            auto parent_sid = parent_stmt.id();
            auto child_decl = child_stmt.declaration();

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& child_id : m_pkb->getChildrenOf(parent_sid))
                new_domain.emplace(child_decl, child_id);

            m_table.upsertDomains(child_decl, table::entry_set_intersect(new_domain, m_table.getDomain(child_decl)));
        }
        else if(parent_stmt.isStatementId() && child_stmt.isWildcard())
        {
            util::log("pql::eval", "processing Parent(StmtId, _)");

            auto parent_sid = parent_stmt.id();
            if(m_pkb->getChildrenOf(parent_sid).empty())
                throw PqlException(
                    "pql::eval", "{} is always false ('{}' has no children)", rel->toString(), parent_sid);
        }

        else if(parent_stmt.isDeclaration() && child_stmt.isStatementId())
        {
            util::log("pql::eval", "processing Parent(DeclaredStmt, StmtId)");

            auto parent_decl = parent_stmt.declaration();
            auto child_sid = child_stmt.id();

            auto parent_id = m_pkb->getParentOf(child_sid);
            if(!parent_id.has_value())
                throw PqlException("pql::eval", "{} is always false ('{}' has no parent)", rel->toString(), child_sid);

            std::unordered_set<table::Entry> new_domain { table::Entry(parent_decl, parent_id.value()) };

            m_table.upsertDomains(parent_decl, table::entry_set_intersect(new_domain, m_table.getDomain(parent_decl)));
        }
        else if(parent_stmt.isDeclaration() && child_stmt.isDeclaration())
        {
            util::log("pql::eval", "processing Parent(DeclaredStmt, DeclaredStmt)");

            auto parent_decl = parent_stmt.declaration();
            auto child_decl = child_stmt.declaration();

            // same strategy as Follows
            auto parent_domain = m_table.getDomain(parent_decl);
            auto new_child_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = parent_domain.begin(); it != parent_domain.end();)
            {
                auto children = m_pkb->getChildrenOf(it->getStmtNum());
                if(children.empty())
                {
                    it = parent_domain.erase(it);
                    continue;
                }

                auto p_entry = table::Entry(parent_decl, it->getStmtNum());
                for(auto child_sid : children)
                {
                    auto c_entry = table::Entry(child_decl, child_sid);
                    new_child_domain.insert(c_entry);
                    allowed_entries.insert({ p_entry, c_entry });
                }
                ++it;
            }

            m_table.upsertDomains(parent_decl, parent_domain);
            m_table.upsertDomains(
                child_decl, table::entry_set_intersect(new_child_domain, m_table.getDomain(child_decl)));
            m_table.addJoin(table::Join(parent_decl, child_decl, allowed_entries));
        }
        else if(parent_stmt.isDeclaration() && child_stmt.isWildcard())
        {
            util::log("pql::eval", "processing Parent(DeclaredStmt, _)");

            auto parent_decl = parent_stmt.declaration();
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table.getDomain(parent_decl))
            {
                if(m_pkb->getChildrenOf(entry.getStmtNum()).empty())
                    continue;

                new_domain.emplace(parent_decl, entry.getStmtNum());
            }

            m_table.upsertDomains(parent_decl, table::entry_set_intersect(new_domain, m_table.getDomain(parent_decl)));
        }

        else if(parent_stmt.isWildcard() && child_stmt.isStatementId())
        {
            util::log("pql::eval", "processing Parent(_, StmtId)");

            auto child_sid = child_stmt.id();
            if(!m_pkb->getParentOf(child_sid).has_value())
                throw PqlException("pql::eval", "{} is always false ('{}' has no parent)", rel->toString(), child_sid);
        }
        else if(parent_stmt.isWildcard() && child_stmt.isDeclaration())
        {
            util::log("pql::eval", "processing Parent(_, DeclaredStmt)");

            auto child_decl = child_stmt.declaration();
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table.getDomain(child_decl))
            {
                if(!m_pkb->getParentOf(entry.getStmtNum()).has_value())
                    continue;

                new_domain.emplace(child_decl, entry.getStmtNum());
            }

            m_table.upsertDomains(child_decl, table::entry_set_intersect(new_domain, m_table.getDomain(child_decl)));
        }
        else if(parent_stmt.isWildcard() && child_stmt.isWildcard())
        {
            util::log("pql::eval", "Processing Parent(_, _)");
            if(!m_pkb->parentRelationExists())
                throw PqlException("pql::eval", "{} is always false (no consecutive statements)", rel->toString());
        }

        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }




    void Evaluator::handleParentT(const ast::ParentT* rel)
    {
        assert(rel);

        const auto& ancestor_stmt = rel->ancestor;
        const auto& descendant_stmt = rel->descendant;

        if(ancestor_stmt.isDeclaration())
            m_table.addSelectDecl(ancestor_stmt.declaration());

        if(descendant_stmt.isDeclaration())
            m_table.addSelectDecl(descendant_stmt.declaration());

        if(ancestor_stmt.isStatementId() && descendant_stmt.isStatementId())
        {
            util::log("pql::eval", "processing Parent*(StmtId, StmtId)");

            auto parent_sid = ancestor_stmt.id();
            auto child_sid = descendant_stmt.id();

            if(!m_pkb->isParentT(parent_sid, child_sid))
                throw PqlException("pql::eval", "{} is always false", rel->toString());
        }
        else if(ancestor_stmt.isStatementId() && descendant_stmt.isDeclaration())
        {
            util::log("pql::eval", "processing Parent*(StmtId, DeclaredStmt)");

            auto parent_sid = ancestor_stmt.id();
            auto child_decl = descendant_stmt.declaration();

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& child_id : m_pkb->getDescendantsOf(parent_sid))
                new_domain.emplace(child_decl, child_id);

            m_table.upsertDomains(child_decl, table::entry_set_intersect(new_domain, m_table.getDomain(child_decl)));
        }
        else if(ancestor_stmt.isStatementId() && descendant_stmt.isWildcard())
        {
            util::log("pql::eval", "processing Parent*(StmtId, _)");

            auto parent_sid = ancestor_stmt.id();
            if(m_pkb->getDescendantsOf(parent_sid).empty())
                throw PqlException(
                    "pql::eval", "{} is always false ('{}' has no children)", rel->toString(), parent_sid);
        }

        else if(ancestor_stmt.isDeclaration() && descendant_stmt.isStatementId())
        {
            util::log("pql::eval", "processing Parent*(DeclaredStmt, StmtId)");

            auto parent_decl = ancestor_stmt.declaration();
            auto child_sid = descendant_stmt.id();

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& parent_id : m_pkb->getAncestorsOf(child_sid))
                new_domain.emplace(parent_decl, parent_id);

            m_table.upsertDomains(parent_decl, table::entry_set_intersect(new_domain, m_table.getDomain(parent_decl)));
        }
        else if(ancestor_stmt.isDeclaration() && descendant_stmt.isDeclaration())
        {
            util::log("pql::eval", "processing Parent*(DeclaredStmt, DeclaredStmt)");

            auto parent_decl = ancestor_stmt.declaration();
            auto child_decl = descendant_stmt.declaration();

            auto parent_domain = m_table.getDomain(parent_decl);
            auto new_child_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = parent_domain.begin(); it != parent_domain.end();)
            {
                auto children = m_pkb->getDescendantsOf(it->getStmtNum());
                if(children.empty())
                {
                    it = parent_domain.erase(it);
                    continue;
                }

                auto p_entry = table::Entry(parent_decl, it->getStmtNum());
                for(auto child_sid : children)
                {
                    auto c_entry = table::Entry(child_decl, child_sid);
                    new_child_domain.insert(c_entry);
                    allowed_entries.insert({ p_entry, c_entry });
                }
                ++it;
            }

            m_table.upsertDomains(parent_decl, parent_domain);
            m_table.upsertDomains(
                child_decl, table::entry_set_intersect(new_child_domain, m_table.getDomain(child_decl)));
            m_table.addJoin(table::Join(parent_decl, child_decl, allowed_entries));
        }
        else if(ancestor_stmt.isDeclaration() && descendant_stmt.isWildcard())
        {
            util::log("pql::eval", "processing Parent*(DeclaredStmt, _)");

            auto parent_decl = ancestor_stmt.declaration();
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table.getDomain(parent_decl))
            {
                if(m_pkb->getDescendantsOf(entry.getStmtNum()).empty())
                    continue;

                new_domain.emplace(parent_decl, entry.getStmtNum());
            }

            m_table.upsertDomains(parent_decl, table::entry_set_intersect(new_domain, m_table.getDomain(parent_decl)));
        }

        else if(ancestor_stmt.isWildcard() && descendant_stmt.isStatementId())
        {
            util::log("pql::eval", "processing Parent*(_, StmtId)");

            auto child_sid = descendant_stmt.id();
            if(m_pkb->getAncestorsOf(child_sid).empty())
                throw PqlException("pql::eval", "{} is always false ('{}' has no parent)", rel->toString(), child_sid);
        }
        else if(ancestor_stmt.isWildcard() && descendant_stmt.isDeclaration())
        {
            util::log("pql::eval", "processing Parent*(_, DeclaredStmt)");

            auto child_decl = descendant_stmt.declaration();
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table.getDomain(child_decl))
            {
                if(m_pkb->getAncestorsOf(entry.getStmtNum()).empty())
                    continue;

                new_domain.emplace(child_decl, entry.getStmtNum());
            }

            m_table.upsertDomains(child_decl, table::entry_set_intersect(new_domain, m_table.getDomain(child_decl)));
        }
        else if(ancestor_stmt.isWildcard() && descendant_stmt.isWildcard())
        {
            util::log("pql::eval", "Processing Parent*(_, _)");
            if(!m_pkb->parentRelationExists())
                throw PqlException("pql::eval", "{} is always false (no consecutive statements)", rel->toString());
        }

        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }
}
