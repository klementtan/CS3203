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

    using Abstractor = eval::RelationAbstractor<Procedure, std::string, EntRef, /* SetsAreConstRef: */ true>;

    void Calls::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto {
            Abstractor abs {};
            abs.relationName = "Calls";
            abs.leftDeclEntity = DESIGN_ENT::PROCEDURE;
            abs.rightDeclEntity = DESIGN_ENT::PROCEDURE;

            abs.relationHolds = [](const ProgramKB* pkb, const Procedure& a, const Procedure& b) -> bool {
                return a.callsProcedure(b.getName());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Procedure& a, const Procedure& b) -> bool {
                return a.isCalledByProcedure(b.getName());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Procedure& p) -> decltype(auto) {
                return p.getAllCalledProcedures();
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Procedure& p) -> decltype(auto) {
                return p.getAllCallers();
            };

            abs.relationExists = &ProgramKB::callsRelationExists;
            abs.getEntity = &ProgramKB::getProcedureNamed;
            return abs;
        }();

        abs.evaluate(pkb, tbl, this, &this->caller, &this->proc);
    }

    void CallsT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        // see the comment in follows.cpp
        static auto abs = []() -> auto {
            Abstractor abs {};
            abs.relationName = "Calls*";
            abs.leftDeclEntity = DESIGN_ENT::PROCEDURE;
            abs.rightDeclEntity = DESIGN_ENT::PROCEDURE;

            abs.relationHolds = [](const ProgramKB* pkb, const Procedure& a, const Procedure& b) -> bool {
                return a.callsProcedureTransitively(b.getName());
            };

            abs.inverseRelationHolds = [](const ProgramKB* pkb, const Procedure& a, const Procedure& b) -> bool {
                return a.isTransitivelyCalledByProcedure(b.getName());
            };

            abs.getAllRelated = [](const ProgramKB* pkb, const Procedure& p) -> decltype(auto) {
                return p.getAllTransitivelyCalledProcedures();
            };

            abs.getAllInverselyRelated = [](const ProgramKB* pkb, const Procedure& p) -> decltype(auto) {
                return p.getAllTransitiveCallers();
            };

            abs.relationExists = &ProgramKB::callsRelationExists;
            abs.getEntity = &ProgramKB::getProcedureNamed;
            return abs;
        }();

        abs.evaluate(pkb, tbl, this, &this->caller, &this->proc);
    }
}
