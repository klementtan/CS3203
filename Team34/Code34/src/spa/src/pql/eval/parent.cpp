// parent.cpp

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

    void Parent::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        spa_assert(pkb);
        spa_assert(tbl);

        static auto abs = []() -> auto
        {
            Abstractor abs {};

            abs.relationName = "Parent";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return a.isParentOf(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return a.isChildOf(b.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return s.getChildren();
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return s.getParent();
            };

            abs.relationExists = &ProgramKB::parentRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }
        ();

        abs.evaluate(pkb, tbl, this, &this->parent, &this->child);
    }

    void ParentT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        spa_assert(pkb);
        spa_assert(tbl);

        static auto abs = []() -> auto
        {
            Abstractor abs {};

            abs.relationName = "Parent*";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return a.isAncestorOf(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return a.isDescendantOf(b.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return s.getDescendants();
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return s.getAncestors();
            };

            abs.relationExists = &ProgramKB::parentRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }
        ();

        abs.evaluate(pkb, tbl, this, &this->ancestor, &this->descendant);
    }
}
