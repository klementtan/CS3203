// parent.cpp

#include <cassert>
#include <algorithm>

#include "pql/exception.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = pql::exception::PqlException;

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
            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->parent);
            auto child_sid = dynamic_cast<ast::StmtId*>(rel->child);

            if(!m_pkb->isParent(parent_sid, child_sid))
                throw PqlException("pql::eval", "{} is always false", rel->toString());
        }
        else if(is_parent_sid && is_child_decl)
        {
            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->parent);
            auto child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->child);

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& child_id : m_pkb->getChildrenOf(parent_sid->id))
                new_domain.emplace(child_decl->declaration, child_id);

            m_table->upsertDomains(child_decl->declaration,
                table::entry_set_intersect(new_domain, m_table->getDomain(child_decl->declaration)));
        }
        else if(is_parent_sid && is_child_wildcard)
        {
            auto parent_sid = dynamic_cast<ast::StmtId*>(rel->parent);
            if(m_pkb->getChildrenOf(parent_sid->id).empty())
                throw PqlException("pql::eval", "{} is always false ('{}' has no children)", rel->toString(), parent_sid->id);
        }

        else if(is_parent_decl && is_child_sid)
        {
            auto parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->parent);
            auto child_sid = dynamic_cast<ast::StmtId*>(rel->child);

            auto parent_id = m_pkb->getParentOf(child_sid->id);
            if(parent_id.empty())
                throw PqlException("pql::eval", "{} is always false ('{}' has no parent)", rel->toString(), child_sid->id);

            std::unordered_set<table::Entry> new_domain {
                table::Entry(parent_decl->declaration, parent_id.value())
            };

            m_table->upsertDomains(parent_decl->declaration,
                table::entry_set_intersect(new_domain, m_table->getDomain(parent_decl->declaration)));
        }
        else if(is_parent_decl && is_child_decl)
        {
            auto parent_decl = dynamic_cast<ast::DeclaredStmt*>(rel->parent);
            auto child_decl = dynamic_cast<ast::DeclaredStmt*>(rel->child);
        }
    }

    void Evaluator::handleParentT(const ast::ParentT* parent_t) { }
}
