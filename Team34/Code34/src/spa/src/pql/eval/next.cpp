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

    using Abstractor = eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ false>;

    void Next::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto {
            Abstractor abs {};
            abs.relationName = "Next";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->isStatementNext(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->isStatementNext(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getNextStatements(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getPreviousStatements(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::nextRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }();

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }



    void NextT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto {
            Abstractor abs {};
            abs.relationName = "Next*";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->isStatementTransitivelyNext(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->isStatementTransitivelyNext(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getTransitivelyNextStatements(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getTransitivelyPreviousStatements(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::nextRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }();

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }
}
