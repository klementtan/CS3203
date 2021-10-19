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

    void Affects::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        static eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ false> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Affects";
            abs.leftDeclEntity = DESIGN_ENT::ASSIGN;
            abs.rightDeclEntity = DESIGN_ENT::ASSIGN;

            abs.relationHolds = [pkb](const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesAffect(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [pkb](const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesAffect(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [pkb](const Statement& s) -> StatementSet {
                return pkb->getCFG()->getAffectedStatements(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [pkb](const Statement& s) -> StatementSet {
                return pkb->getCFG()->getAffectingStatements(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::affectsRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
        }
        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }

    void AffectsT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        static eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ true> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Affects*";
            abs.leftDeclEntity = DESIGN_ENT::ASSIGN;
            abs.rightDeclEntity = DESIGN_ENT::ASSIGN;

            abs.relationHolds = [pkb](const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesTransitivelyAffect(a.getStmtNum(), b.getStmtNum());
            };

            abs.inverseRelationHolds = [pkb](const Statement& a, const Statement& b) -> bool {
                return pkb->getCFG()->doesTransitivelyAffect(b.getStmtNum(), a.getStmtNum());
            };

            abs.getAllRelated = [pkb](const Statement& s) -> StatementSet {
                return pkb->getCFG()->getTransitivelyAffectedStatements(s.getStmtNum());
            };

            abs.getAllInverselyRelated = [pkb](const Statement& s) -> StatementSet {
                return pkb->getCFG()->getTransitivelyAffectingStatements(s.getStmtNum());
            };

            abs.relationExists = &ProgramKB::affectsRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
        }
        abs.evaluate(pkb, tbl, this, &this->first, &this->second);
    }
}
