// statement.cpp

#include <cassert>
#include "pkb.h"

namespace pkb
{
    Statement::Statement(const simple::ast::Stmt* stmt) : m_stmt(stmt) { }

    const simple::ast::Stmt* Statement::getAstStmt() const
    {
        return m_stmt;
    }

    simple::ast::StatementNum Statement::getStmtNum() const
    {
        return m_stmt->id;
    }

    bool Statement::hasFollower() const
    {
        return m_directly_after != 0;
    }

    bool Statement::isFollower() const
    {
        return m_directly_before != 0;
    }

    bool Statement::doesFollow(StatementNum id) const
    {
        return m_directly_before == id;
    }

    bool Statement::doesFollowTransitively(StatementNum id) const
    {
        return m_before.count(id) > 0;
    }

    bool Statement::isFollowedBy(StatementNum id) const
    {
        return m_directly_after == id;
    }

    bool Statement::isFollowedTransitivelyBy(StatementNum id) const
    {
        return m_after.count(id) > 0;
    }

    simple::ast::StatementNum Statement::getStmtDirectlyAfter() const
    {
        return m_directly_after;
    }

    simple::ast::StatementNum Statement::getStmtDirectlyBefore() const
    {
        return m_directly_before;
    }

    const std::unordered_set<simple::ast::StatementNum>& Statement::getStmtsTransitivelyAfter() const
    {
        return m_after;
    }

    const std::unordered_set<simple::ast::StatementNum>& Statement::getStmtsTransitivelyBefore() const
    {
        return m_before;
    }

    bool Statement::usesVariable(const std::string& var_name) const
    {
        return m_uses.count(var_name) > 0;
    }

    bool Statement::modifiesVariable(const std::string& var_name) const
    {
        return m_modifies.count(var_name) > 0;
    }

    const std::unordered_set<std::string>& Statement::getUsedVariables() const
    {
        return m_uses;
    }

    const std::unordered_set<std::string>& Statement::getModifiedVariables() const
    {
        return m_modifies;
    }


    bool Statement::isParentOf(StatementNum id) const
    {
        return m_children.count(id) > 0;
    }

    bool Statement::isAncestorOf(StatementNum id) const
    {
        return m_descendants.count(id) > 0;
    }

    bool Statement::isChildOf(StatementNum id) const
    {
        return m_parent.count(id) > 0;
    }

    bool Statement::isDescendantOf(StatementNum id) const
    {
        return m_ancestors.count(id) > 0;
    }

    const StatementSet& Statement::getChildren() const
    {
        return m_children;
    }

    const StatementSet& Statement::getDescendants() const
    {
        return m_descendants;
    }

    const StatementSet& Statement::getParent() const
    {
        assert(m_parent.size() <= 1);
        return m_parent;
    }

    const StatementSet& Statement::getAncestors() const
    {
        return m_ancestors;
    }

    bool Statement::isStatementNext(StatementNum id) const
    {
        return m_directly_nexts.count(id) > 0;
    };
    bool Statement::isStatementTransitivelyNext(StatementNum id) const
    {
        zpr::print("it is fsf");
        for(auto a : m_nexts)
        {
            zpr::print("it is {}", a);
        }
        return m_nexts.count(id) > 0;
    };
    const StatementSet& Statement::getNextStatements(StatementNum id) const
    {
        return m_directly_nexts;
    };
    const StatementSet& Statement::getTransitivelyNextStatements(StatementNum id) const
    {
        return m_nexts;
    };

    const std::unordered_set<std::string>& Statement::getVariablesUsedInCondition() const
    {
        return m_condition_uses;
    }
}
