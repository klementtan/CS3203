// statement.cpp

#include "pkb.h"
#include "exceptions.h"

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
        spa_assert(m_parent.size() <= 1);
        return m_parent;
    }

    const StatementSet& Statement::getAncestors() const
    {
        return m_ancestors;
    }

    const std::unordered_set<std::string>& Statement::getVariablesUsedInCondition() const
    {
        return m_condition_uses;
    }


    void Statement::resetCache() const
    {
        m_did_cache_next = false;
        m_did_cache_prev = false;
        m_did_cache_transitively_next = false;
        m_did_cache_transitively_prev = false;

        m_did_cache_affects = false;
        m_did_cache_affecting = false;
        m_did_cache_transitively_affects = false;
        m_did_cache_transitively_affecting = false;

        m_did_cache_next_bip = false;
        m_did_cache_prev_bip = false;
        m_did_cache_transitively_next_bip = false;
        m_did_cache_transitively_prev_bip = false;
    }

    const simple::ast::Procedure* Statement::getProc() const
    {
        return proc;
    }


    const StatementSet* Statement::maybeGetNextStatements() const
    {
        if(m_did_cache_next)
            return &m_next;

        return nullptr;
    }

    const StatementSet& Statement::cacheNextStatements(StatementSet stmts) const
    {
        m_next = std::move(stmts);
        m_did_cache_next = true;

        return m_next;
    }

    const StatementSet* Statement::maybeGetPreviousStatements() const
    {
        if(m_did_cache_prev)
            return &m_prev;

        return nullptr;
    }

    const StatementSet& Statement::cachePreviousStatements(StatementSet stmts) const
    {
        m_prev = std::move(stmts);
        m_did_cache_prev = true;

        return m_prev;
    }

    const StatementSet* Statement::maybeGetTransitivelyNextStatements() const
    {
        if(m_did_cache_transitively_next)
            return &m_transitively_next;

        return nullptr;
    }

    const StatementSet& Statement::cacheTransitivelyNextStatements(StatementSet stmts) const
    {
        m_transitively_next = std::move(stmts);
        m_did_cache_transitively_next = true;

        return m_transitively_next;
    }


    const StatementSet* Statement::maybeGetTransitivelyPreviousStatements() const
    {
        if(m_did_cache_transitively_prev)
            return &m_transitively_prev;

        return nullptr;
    }

    const StatementSet& Statement::cacheTransitivelyPreviousStatements(StatementSet stmts) const
    {
        m_transitively_prev = std::move(stmts);
        m_did_cache_transitively_prev = true;

        return m_transitively_prev;
    }



    const StatementSet* Statement::maybeGetAffectedStatements() const
    {
        if(m_did_cache_affects)
            return &m_affects;

        return nullptr;
    }

    const StatementSet& Statement::cacheAffectedStatements(StatementSet stmts) const
    {
        m_affects = std::move(stmts);
        m_did_cache_affects = true;

        return m_affects;
    }

    const StatementSet* Statement::maybeGetAffectingStatements() const
    {
        if(m_did_cache_affecting)
            return &m_affecting;

        return nullptr;
    }

    const StatementSet& Statement::cacheAffectingStatements(StatementSet stmts) const
    {
        m_affecting = std::move(stmts);
        m_did_cache_affecting = true;

        return m_affecting;
    }


    const StatementSet* Statement::maybeGetTransitivelyAffectedStatements() const
    {
        if(m_did_cache_transitively_affects)
            return &m_transitively_affects;

        return nullptr;
    }

    const StatementSet* Statement::maybeGetTransitivelyAffectingStatements() const
    {
        if(m_did_cache_transitively_affecting)
            return &m_transitively_affecting;

        return nullptr;
    }

    const StatementSet* Statement::maybeGetNextStatementsBip() const
    {
        if(m_did_cache_next_bip)
            return &m_next_bip;

        return nullptr;
    }

    const StatementSet& Statement::cacheNextStatementsBip(StatementSet stmts) const
    {
        m_next_bip = std::move(stmts);
        m_did_cache_next_bip = true;

        return m_next_bip;
    }

    const StatementSet* Statement::maybeGetPreviousStatementsBip() const
    {
        if(m_did_cache_prev_bip)
            return &m_prev_bip;

        return nullptr;
    }

    const StatementSet& Statement::cachePreviousStatementsBip(StatementSet stmts) const
    {
        m_prev_bip = std::move(stmts);
        m_did_cache_prev_bip = true;

        return m_prev_bip;
    }

    const StatementSet* Statement::maybeGetTransitivelyNextStatementsBip() const
    {
        if(m_did_cache_transitively_next_bip)
            return &m_transitively_next_bip;

        return nullptr;
    }

    const StatementSet& Statement::cacheTransitivelyNextStatementsBip(StatementSet stmts) const
    {
        m_transitively_next_bip = std::move(stmts);
        m_did_cache_transitively_next_bip = true;

        return m_transitively_next_bip;
    }


    const StatementSet* Statement::maybeGetTransitivelyPreviousStatementsBip() const
    {
        if(m_did_cache_transitively_prev_bip)
            return &m_transitively_prev_bip;

        return nullptr;
    }

    const StatementSet& Statement::cacheTransitivelyPreviousStatementsBip(StatementSet stmts) const
    {
        m_transitively_prev_bip = std::move(stmts);
        m_did_cache_transitively_prev_bip = true;

        return m_transitively_prev_bip;
    }

    const StatementSet& Statement::cacheTransitivelyAffectedStatements(StatementSet stmts) const
    {
        m_transitively_affects = std::move(stmts);
        m_did_cache_transitively_affects = true;

        return m_transitively_affects;
    }

    const StatementSet& Statement::cacheTransitivelyAffectingStatements(StatementSet stmts) const
    {
        m_transitively_affecting = std::move(stmts);
        m_did_cache_transitively_affecting = true;

        return m_transitively_affecting;
    }
}
