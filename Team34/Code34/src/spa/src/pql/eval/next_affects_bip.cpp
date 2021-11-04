// next_affects_bip.cpp

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

    using Abstractor = eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ true>;

	void NextBip::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        spa_assert(pkb);
        spa_assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto
        {
            Abstractor abs {};
            abs.relationName = "NextBip";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->isStatementNextBip(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->isStatementNextBip(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getNextStatementsBip(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getPreviousStatementsBip(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::nextBipRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }
        ();

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }


    void NextBipT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        spa_assert(pkb);
        spa_assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto
        {
            Abstractor abs {};
            abs.relationName = "NextBipT";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->isStatementTransitivelyNextBip(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->isStatementTransitivelyNextBip(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getTransitivelyNextStatementsBip(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getTransitivelyPreviousStatementsBip(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::nextBipRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }
        ();

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }


    void AffectsBip::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        spa_assert(pkb);
        spa_assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto
        {
            Abstractor abs {};
            abs.relationName = "AffectsBip";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesAffectBip(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesAffectBip(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getAffectedStatementsBip(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getAffectingStatementsBip(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::affectsBipRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }
        ();

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }


    void AffectsBipT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        spa_assert(pkb);
        spa_assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto
        {
            Abstractor abs {};
            abs.relationName = "AffectsBipT";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesTransitivelyAffectBip(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesTransitivelyAffectBip(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getTransitivelyAffectedStatementsBip(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return pkb->getCFG()->getTransitivelyAffectingStatementsBip(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::affectsBipRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }
        ();

        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }
}