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

        // note: this is a bit of a problem, because the checkers must capture the pkb, and
        // the "common evaluator" does not pass in the pkb. this means the lambdas must be
        // re-created for each pkb, which means they can't be statically constructed.
        // TODO(#175)

        RelationAbstractor<Statement, StatementNum, ast::StmtRef, /* SetsAreConstRef: */ false> abs {};
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
            return m_pkb->getCFG()->getPreviousStatements(s.getStmtNum());
        };

        abs.relationExists = &pkb::ProgramKB::nextRelationExists;
        abs.getEntity = &pkb::ProgramKB::getStatementAt;

        abs.evaluate(m_pkb, &m_table, rel, &rel->first, &rel->second);
    }



    void Evaluator::handleNextT(const ast::NextT* rel)
    {
        assert(rel);

        RelationAbstractor<Statement, StatementNum, ast::StmtRef, /* SetsAreConstRef: */ false> abs {};
        abs.relationName = "Next*";
        abs.leftDeclEntity = {};
        abs.rightDeclEntity = {};

        abs.relationHolds = [this](const Statement& a, const Statement& b) -> bool {
            return m_pkb->getCFG()->isStatementTransitivelyNext(a.getStmtNum(), b.getStmtNum());
        };

        abs.inverseRelationHolds = [this](const Statement& a, const Statement& b) -> bool {
            return m_pkb->getCFG()->isStatementTransitivelyNext(b.getStmtNum(), a.getStmtNum());
        };

        abs.getAllRelated = [this](const Statement& s) -> decltype(auto) {
            return m_pkb->getCFG()->getTransitivelyNextStatements(s.getStmtNum());
        };

        abs.getAllInverselyRelated = [this](const Statement& s) -> decltype(auto) {
            return m_pkb->getCFG()->getTransitivelyPreviousStatements(s.getStmtNum());
        };

        abs.relationExists = &pkb::ProgramKB::nextRelationExists;
        abs.getEntity = &pkb::ProgramKB::getStatementAt;

        abs.evaluate(m_pkb, &m_table, rel, &rel->first, &rel->second);
    }
}
