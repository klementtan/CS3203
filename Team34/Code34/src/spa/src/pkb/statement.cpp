// statement.cpp

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

    bool Statement::follows(simple::ast::StatementNum id) const
    {
        return m_directly_before == id;
    }

    bool Statement::followsTransitively(simple::ast::StatementNum id) const
    {
        return m_before.count(id) > 0;
    }

    bool Statement::followedBy(simple::ast::StatementNum id) const
    {
        return m_directly_after == id;
    }

    bool Statement::followedTransitivelyBy(simple::ast::StatementNum id) const
    {
        return m_after.count(id) > 0;
    }

    simple::ast::StatementNum Statement::getDirectFollower() const
    {
        return m_directly_after;
    }

    simple::ast::StatementNum Statement::getDirectFollowee() const
    {
        return m_directly_before;
    }

    const std::unordered_set<simple::ast::StatementNum>& Statement::getTransitiveFollowers() const
    {
        return m_after;
    }

    const std::unordered_set<simple::ast::StatementNum>& Statement::getTransitiveFollowees() const
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
}
