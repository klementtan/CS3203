// affects.cpp

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

    void Affects::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto {
            Abstractor abs {};
            abs.relationName = "Affects";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesAffect(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesAffect(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> StatementSet {
                return pkb->getCFG()->getAffectedStatements(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> StatementSet {
                return pkb->getCFG()->getAffectingStatements(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::affectsRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }();

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }

    void AffectsT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto {
            Abstractor abs {};
            abs.relationName = "Affects*";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesTransitivelyAffect(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesTransitivelyAffect(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> StatementSet {
                return pkb->getCFG()->getTransitivelyAffectedStatements(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> StatementSet {
                return pkb->getCFG()->getTransitivelyAffectingStatements(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::affectsRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }();

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }
}
