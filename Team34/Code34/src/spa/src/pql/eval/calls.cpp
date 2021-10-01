// follows.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/common.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using namespace pkb;

    void Evaluator::handleCalls(const ast::Calls* rel)
    {
        assert(rel);

        static RelationAbstractor<pkb::Procedure, std::string, ast::EntRef, /* SetsAreConstRef: */ true> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Calls";
            abs.leftDeclEntity = ast::DESIGN_ENT::PROCEDURE;
            abs.rightDeclEntity = ast::DESIGN_ENT::PROCEDURE;

            abs.relationHolds = [](const Procedure& a, const Procedure& b) -> bool {
                return a.callsProcedure(b.getName());
            };

            abs.inverseRelationHolds = [](const Procedure& a, const Procedure& b) -> bool {
                return a.isCalledByProcedure(b.getName());
            };

            abs.getAllRelated = [](const Procedure& p) -> decltype(auto) {
                return p.getAllCalledProcedures();
            };

            abs.getAllInverselyRelated = [](const Procedure& p) -> decltype(auto) {
                return p.getAllCallers();
            };

            abs.relationExists = &pkb::ProgramKB::callsRelationExists;
            abs.getEntity = &pkb::ProgramKB::getProcedureNamed;
        }

        abs.evaluate(m_pkb, &m_table, rel, &rel->caller, &rel->proc);
    }


    void Evaluator::handleCallsT(const ast::CallsT* rel)
    {
        assert(rel);

        static RelationAbstractor<pkb::Procedure, std::string, ast::EntRef, /* SetsAreConstRef: */ true> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Calls*";
            abs.leftDeclEntity = ast::DESIGN_ENT::PROCEDURE;
            abs.rightDeclEntity = ast::DESIGN_ENT::PROCEDURE;

            abs.relationHolds = [](const Procedure& a, const Procedure& b) -> bool {
                return a.callsProcedureTransitively(b.getName());
            };

            abs.inverseRelationHolds = [](const Procedure& a, const Procedure& b) -> bool {
                return a.isTransitivelyCalledByProcedure(b.getName());
            };

            abs.getAllRelated = [](const Procedure& p) -> decltype(auto) {
                return p.getAllTransitivelyCalledProcedures();
            };

            abs.getAllInverselyRelated = [](const Procedure& p) -> decltype(auto) {
                return p.getAllTransitiveCallers();
            };

            abs.relationExists = &pkb::ProgramKB::callsRelationExists;
            abs.getEntity = &pkb::ProgramKB::getProcedureNamed;
        }

        abs.evaluate(m_pkb, &m_table, rel, &rel->caller, &rel->proc);
    }
}
