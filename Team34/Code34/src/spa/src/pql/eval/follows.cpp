// follows.cpp

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

    void Follows::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // note: the reason these aren't just virtal methods is not just me not using OOP out of spite,
        // but because it would require a little more template magic (because of the relations being
        // able to return const-refs (or not)) than I want; it's easier this way, trust me.

        using Abstractor = eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ false>;
        static auto abs = []() -> auto {
            Abstractor abs {};

            abs.relationName = "Follows";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return a.isFollowedBy(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return a.doesFollow(b.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> StatementSet {
                if(auto tmp = s.getStmtDirectlyAfter(); tmp != 0)
                    return { tmp };
                else
                    return {};
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> StatementSet {
                if(auto tmp = s.getStmtDirectlyBefore(); tmp != 0)
                    return { tmp };
                else
                    return {};
            };

            abs.relationExists = &ProgramKB::followsRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;

            return abs;
        }();

        abs.evaluate(pkb, tbl, this, &this->directly_before, &this->directly_after);
    }

    void FollowsT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // see the comment above
        using Abstractor = eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ true>;
        static auto abs = []() -> auto {
            Abstractor abs {};
            abs.relationName = "Follows*";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return a.isFollowedTransitivelyBy(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Statement& a, const Statement& b) -> bool {
                return a.doesFollowTransitively(b.getStmtNum());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return s.getStmtsTransitivelyAfter();
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Statement& s) -> decltype(auto) {
                return s.getStmtsTransitivelyBefore();
            };

            abs.relationExists = &ProgramKB::followsRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
            return abs;
        }();

        abs.evaluate(pkb, tbl, this, &this->before, &this->after);
    }
}
