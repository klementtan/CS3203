// next.cpp

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
    using StatementSet = std::unordered_set<StatementNum>;

    void Evaluator::handleNext(const ast::Next* rel)
    {
        assert(rel);

        static RelationAbstractor<Statement, StatementNum, ast::StmtRef, /* SetsAreConstRef: */ false> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Next";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [this](const Statement& a, const Statement& b) -> bool {
                return m_pkb->getCFG()->isStatementNext(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [this](const Statement& a, const Statement& b) -> bool {
                return m_pkb->getCFG()->isStatementNext(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [this](const Statement& s) -> decltype(auto) {
                return m_pkb->getCFG()->getNextStatements(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [this](const Statement& s) -> decltype(auto) {
                // return s.getParent();
                return StatementSet{};
            };

            abs.relationExists = &pkb::ProgramKB::nextRelationExists;
            abs.getEntity = &pkb::ProgramKB::getStatementAt;
        }
        abs.evaluate(m_pkb, &m_table, rel, &rel->first, &rel->second);
    }



#if 0
    void Evaluator::handleNextT(const ast::NextT* rel)
    {
        assert(rel);

        static RelationAbstractor<Statement, StatementNum, ast::StmtRef, /* SetsAreConstRef: */ false> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Next*";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.isAncestorOf(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.isDescendantOf(b.getStmtNum());
            };

            abs.getAllRelated = [](const Statement& s) -> decltype(auto) {
                return s.getDescendants();
            };

            abs.getAllInverselyRelated = [](const Statement& s) -> decltype(auto) {
                return s.getAncestors();
            };

            abs.relationExists = &pkb::ProgramKB::parentRelationExists;
            abs.getEntity = &pkb::ProgramKB::getStatementAt;
        }
        abs.evaluate(m_pkb, &m_table, rel, &rel->ancestor, &rel->descendant);
    }
#endif
}
