// follows.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/common.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using namespace pkb;
    using PqlException = util::PqlException;

    void Evaluator::handleFollows(const ast::Follows* rel)
    {
        assert(rel);

        static RelationAbstractor<Statement, StatementNum, ast::StmtRef, /* SetsAreConstRef: */ false> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Follows";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.isFollowedBy(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.doesFollow(b.getStmtNum());
            };

            abs.getAllRelated = [](const Statement& s) -> StatementSet {
                if(auto tmp = s.getStmtDirectlyAfter(); tmp != 0)
                    return { tmp };
                else
                    return {};
            };

            abs.getAllInverselyRelated = [](const Statement& s) -> StatementSet {
                if(auto tmp = s.getStmtDirectlyBefore(); tmp != 0)
                    return { tmp };
                else
                    return {};
            };

            abs.relationExists = &pkb::ProgramKB::followsRelationExists;
            abs.getEntity = &pkb::ProgramKB::getStatementAt;
        }
        abs.evaluate(m_pkb, &m_table, rel, &rel->directly_before, &rel->directly_after);
    }




    void Evaluator::handleFollowsT(const ast::FollowsT* rel)
    {
        assert(rel);

        static RelationAbstractor<Statement, StatementNum, ast::StmtRef, /* SetsAreConstRef: */ true> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Follows*";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.isFollowedTransitivelyBy(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.doesFollowTransitively(b.getStmtNum());
            };

            abs.getAllRelated = [](const Statement& s) -> decltype(auto) {
                return s.getStmtsTransitivelyAfter();
            };

            abs.getAllInverselyRelated = [](const Statement& s) -> decltype(auto) {
                return s.getStmtsTransitivelyBefore();
            };

            abs.relationExists = &pkb::ProgramKB::followsRelationExists;
            abs.getEntity = &pkb::ProgramKB::getStatementAt;
        }
        abs.evaluate(m_pkb, &m_table, rel, &rel->before, &rel->after);
    }
}
