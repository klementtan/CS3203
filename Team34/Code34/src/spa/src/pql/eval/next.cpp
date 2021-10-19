// next.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/common.h"
#include "pql/eval/evaluator.h"

namespace pql::ast
{
    using namespace pkb;
    namespace table = pql::eval::table;

    using PqlException = util::PqlException;

    void Next::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // note: this is a bit of a problem, because the checkers must capture the pkb, and
        // the "common evaluator" does not pass in the pkb. this means the lambdas must be
        // re-created for each pkb, which means they can't be statically constructed.
        // TODO(#175)

        eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ false> abs {};
        abs.relationName = "Next";
        abs.leftDeclEntity = {};
        abs.rightDeclEntity = {};

        abs.relationHolds = [pkb](const Statement& a, const Statement& b) -> bool {
            return pkb->getCFG()->isStatementNext(a.getStmtNum(), b.getStmtNum());
        };

        abs.inverseRelationHolds = [pkb](const Statement& a, const Statement& b) -> bool {
            return pkb->getCFG()->isStatementNext(b.getStmtNum(), a.getStmtNum());
        };

        abs.getAllRelated = [pkb](const Statement& s) -> decltype(auto) {
            return pkb->getCFG()->getNextStatements(s.getStmtNum());
        };

        abs.getAllInverselyRelated = [pkb](const Statement& s) -> decltype(auto) {
            return pkb->getCFG()->getPreviousStatements(s.getStmtNum());
        };

        abs.relationExists = &ProgramKB::nextRelationExists;
        abs.getEntity = &ProgramKB::getStatementAt;

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }



    void NextT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ false> abs {};
        abs.relationName = "Next*";
        abs.leftDeclEntity = {};
        abs.rightDeclEntity = {};

        abs.relationHolds = [pkb](const Statement& a, const Statement& b) -> bool {
            return pkb->getCFG()->isStatementTransitivelyNext(a.getStmtNum(), b.getStmtNum());
        };

        abs.inverseRelationHolds = [pkb](const Statement& a, const Statement& b) -> bool {
            return pkb->getCFG()->isStatementTransitivelyNext(b.getStmtNum(), a.getStmtNum());
        };

        abs.getAllRelated = [pkb](const Statement& s) -> decltype(auto) {
            return pkb->getCFG()->getTransitivelyNextStatements(s.getStmtNum());
        };

        abs.getAllInverselyRelated = [pkb](const Statement& s) -> decltype(auto) {
            return pkb->getCFG()->getTransitivelyPreviousStatements(s.getStmtNum());
        };

        abs.relationExists = &ProgramKB::nextRelationExists;
        abs.getEntity = &ProgramKB::getStatementAt;

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }
}
