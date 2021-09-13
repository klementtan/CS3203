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
        bool is_parent_sid = dynamic_cast<ast::StmtId*>(rel->parent);
        bool is_parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->parent);
        bool is_parent_wildcard = dynamic_cast<ast::AllStmt*>(rel->parent);

        bool is_child_sid = dynamic_cast<ast::StmtId*>(rel->child);
        bool is_child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->child);
        bool is_child_wildcard = dynamic_cast<ast::AllStmt*>(rel->child);

        if(is_parent_sid && is_child_sid)
        {
            util::log("pql::eval", "processing Parent(StmtId, StmtId)");

            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->parent)->id;
            auto child_sid = dynamic_cast<ast::StmtId*>(rel->child)->id;

            if(!m_pkb->isParent(parent_sid, child_sid))
                throw PqlException("pql::eval", "{} is always false", rel->toString());
        }
        else if(is_parent_sid && is_child_decl)
        {
            util::log("pql::eval", "processing Parent(StmtId, DeclaredStmt)");

            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->parent)->id;
            auto child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->child)->declaration;

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& child_id : m_pkb->getChildrenOf(parent_sid))
                new_domain.emplace(child_decl, child_id);

            m_table->upsertDomains(child_decl, table::entry_set_intersect(new_domain, m_table->getDomain(child_decl)));
        }
        else if(is_parent_sid && is_child_wildcard)
        {
            util::log("pql::eval", "processing Parent(StmtId, _)");

            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->parent)->id;
            if(m_pkb->getChildrenOf(parent_sid).empty())
                throw PqlException(
                    "pql::eval", "{} is always false ('{}' has no children)", rel->toString(), parent_sid);
        }

        else if(is_parent_decl && is_child_sid)
        {
            util::log("pql::eval", "processing Parent(DeclaredStmt, StmtId)");

            auto parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->parent)->declaration;
            auto child_sid = dynamic_cast<ast::StmtId*>(rel->child)->id;

            auto parent_id = m_pkb->getParentOf(child_sid);
            if(!parent_id.has_value())
                throw PqlException("pql::eval", "{} is always false ('{}' has no parent)", rel->toString(), child_sid);

            std::unordered_set<table::Entry> new_domain { table::Entry(parent_decl, parent_id.value()) };

            m_table->upsertDomains(
                parent_decl, table::entry_set_intersect(new_domain, m_table->getDomain(parent_decl)));
        }
        else if(is_parent_decl && is_child_decl)
        {
            util::log("pql::eval", "processing Parent(DeclaredStmt, DeclaredStmt)");

            auto parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->parent)->declaration;
            auto child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->child)->declaration;

            // same strategy as Follows
            auto parent_domain = m_table->getDomain(parent_decl);
            auto new_child_domain = table::Domain {};

            for(auto it = parent_domain.begin(); it != parent_domain.end();)
            {
                auto children = m_pkb->getChildrenOf(it->getStmtNum());
                if(children.empty())
                {
                    it = parent_domain.erase(it);
                }
                else
                {
                    auto p_entry = table::Entry(parent_decl, it->getStmtNum());
                    for(auto child_sid : children)
                    {
                        auto c_entry = table::Entry(child_decl, child_sid);
                        m_table->addJoin(p_entry, c_entry);
                        new_child_domain.insert(c_entry);
                    }
                    ++it;
                }
            }

            m_table->upsertDomains(parent_decl, parent_domain);
            m_table->upsertDomains(
                child_decl, table::entry_set_intersect(new_child_domain, m_table->getDomain(child_decl)));
        }
        else if(is_parent_decl && is_child_wildcard)
        {
            util::log("pql::eval", "processing Parent(DeclaredStmt, _)");

            auto parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->parent)->declaration;
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table->getDomain(parent_decl))
            {
                if(m_pkb->getChildrenOf(entry.getStmtNum()).empty())
                    continue;

                new_domain.emplace(parent_decl, entry.getStmtNum());
            }

            m_table->upsertDomains(
                parent_decl, table::entry_set_intersect(new_domain, m_table->getDomain(parent_decl)));
        }

        else if(is_parent_wildcard && is_child_sid)
        {
            util::log("pql::eval", "processing Parent(_, StmtId)");

            auto child_sid = dynamic_cast<ast::StmtId*>(rel->child)->id;
            if(!m_pkb->getParentOf(child_sid).has_value())
                throw PqlException("pql::eval", "{} is always false ('{}' has no parent)", rel->toString(), child_sid);
        }
        else if(is_parent_wildcard && is_child_decl)
        {
            util::log("pql::eval", "processing Parent(_, DeclaredStmt)");

            auto child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->child)->declaration;
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table->getDomain(child_decl))
            {
                if(!m_pkb->getParentOf(entry.getStmtNum()).has_value())
                    continue;

                new_domain.emplace(child_decl, entry.getStmtNum());
            }

            m_table->upsertDomains(child_decl, table::entry_set_intersect(new_domain, m_table->getDomain(child_decl)));
        }
        else if(is_parent_wildcard && is_child_wildcard)
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
        bool is_parent_sid = dynamic_cast<ast::StmtId*>(rel->ancestor);
        bool is_parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->ancestor);
        bool is_parent_wildcard = dynamic_cast<ast::AllStmt*>(rel->ancestor);

        bool is_child_sid = dynamic_cast<ast::StmtId*>(rel->descendant);
        bool is_child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->descendant);
        bool is_child_wildcard = dynamic_cast<ast::AllStmt*>(rel->descendant);

        if(is_parent_sid && is_child_sid)
        {
            util::log("pql::eval", "processing Parent*(StmtId, StmtId)");

            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->ancestor)->id;
            auto child_sid = dynamic_cast<ast::StmtId*>(rel->descendant)->id;

            if(!m_pkb->isParentT(parent_sid, child_sid))
                throw PqlException("pql::eval", "{} is always false", rel->toString());
        }
        else if(is_parent_sid && is_child_decl)
        {
            util::log("pql::eval", "processing Parent*(StmtId, DeclaredStmt)");

            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->ancestor)->id;
            auto child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->descendant)->declaration;

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& child_id : m_pkb->getDescendantsOf(parent_sid))
                new_domain.emplace(child_decl, child_id);

            m_table->upsertDomains(child_decl, table::entry_set_intersect(new_domain, m_table->getDomain(child_decl)));
        }
        else if(is_parent_sid && is_child_wildcard)
        {
            util::log("pql::eval", "processing Parent*(StmtId, _)");

            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->ancestor)->id;
            if(m_pkb->getDescendantsOf(parent_sid).empty())
                throw PqlException(
                    "pql::eval", "{} is always false ('{}' has no children)", rel->toString(), parent_sid);
        }

        else if(is_parent_decl && is_child_sid)
        {
            util::log("pql::eval", "processing Parent*(DeclaredStmt, StmtId)");

            auto parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->ancestor)->declaration;
            auto child_sid = dynamic_cast<ast::StmtId*>(rel->descendant)->id;

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& parent_id : m_pkb->getAncestorsOf(child_sid))
                new_domain.emplace(parent_decl, parent_id);

            m_table->upsertDomains(
                parent_decl, table::entry_set_intersect(new_domain, m_table->getDomain(parent_decl)));
        }
        else if(is_parent_decl && is_child_decl)
        {
            util::log("pql::eval", "processing Parent*(DeclaredStmt, DeclaredStmt)");

            auto parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->ancestor)->declaration;
            auto child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->descendant)->declaration;

            auto parent_domain = m_table->getDomain(parent_decl);
            auto new_child_domain = table::Domain {};

            for(auto it = parent_domain.begin(); it != parent_domain.end();)
            {
                auto children = m_pkb->getDescendantsOf(it->getStmtNum());
                if(children.empty())
                {
                    it = parent_domain.erase(it);
                }
                else
                {
                    auto p_entry = table::Entry(parent_decl, it->getStmtNum());
                    for(auto child_sid : children)
                    {
                        auto c_entry = table::Entry(child_decl, child_sid);
                        m_table->addJoin(p_entry, c_entry);
                        new_child_domain.insert(c_entry);
                    }
                    ++it;
                }
            }

            m_table->upsertDomains(parent_decl, parent_domain);
            m_table->upsertDomains(
                child_decl, table::entry_set_intersect(new_child_domain, m_table->getDomain(child_decl)));
        }
        else if(is_parent_decl && is_child_wildcard)
        {
            util::log("pql::eval", "processing Parent*(DeclaredStmt, _)");

            auto parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->ancestor)->declaration;
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table->getDomain(parent_decl))
            {
                if(m_pkb->getDescendantsOf(entry.getStmtNum()).empty())
                    continue;

                new_domain.emplace(parent_decl, entry.getStmtNum());
            }

            m_table->upsertDomains(
                parent_decl, table::entry_set_intersect(new_domain, m_table->getDomain(parent_decl)));
        }

        else if(is_parent_wildcard && is_child_sid)
        {
            util::log("pql::eval", "processing Parent*(_, StmtId)");

            auto child_sid = dynamic_cast<ast::StmtId*>(rel->descendant)->id;
            if(m_pkb->getAncestorsOf(child_sid).empty())
                throw PqlException("pql::eval", "{} is always false ('{}' has no parent)", rel->toString(), child_sid);
        }
        else if(is_parent_wildcard && is_child_decl)
        {
            util::log("pql::eval", "processing Parent*(_, DeclaredStmt)");

            auto child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->descendant)->declaration;
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table->getDomain(child_decl))
            {
                if(m_pkb->getAncestorsOf(entry.getStmtNum()).empty())
                    continue;

                new_domain.emplace(child_decl, entry.getStmtNum());
            }

            m_table->upsertDomains(child_decl, table::entry_set_intersect(new_domain, m_table->getDomain(child_decl)));
        }
        else if(is_parent_wildcard && is_child_wildcard)
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
