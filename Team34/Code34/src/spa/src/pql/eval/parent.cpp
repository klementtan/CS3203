// parent.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/common.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    void Evaluator::handleParent(const ast::Parent* rel)
    {
        rel->evaluate(m_pkb, &m_table);
    }

    void Evaluator::handleParentT(const ast::ParentT* rel)
    {
        rel->evaluate(m_pkb, &m_table);
    }
}

namespace pql::ast
{
    using namespace pkb;
    namespace table = pql::eval::table;

    using PqlException = util::PqlException;

    void Parent::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        static eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ true> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Parent";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.isParentOf(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.isChildOf(b.getStmtNum());
            };

            abs.getAllRelated = [](const Statement& s) -> decltype(auto) {
                return s.getChildren();
            };

            abs.getAllInverselyRelated = [](const Statement& s) -> decltype(auto) {
                return s.getParent();
            };

            abs.relationExists = &ProgramKB::parentRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
        }
        abs.evaluate(pkb, tbl, this, &this->parent, &this->child);
    }

    void ParentT::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        assert(pkb);
        assert(tbl);

        static eval::RelationAbstractor<Statement, StatementNum, StmtRef, /* SetsAreConstRef: */ true> abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "Parent*";
            abs.leftDeclEntity = {};
            abs.rightDeclEntity = {};

            abs.relationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.isAncestorOf(b.getStmtNum());
            };

            abs.inverseRelationHolds = [](const Statement& a, const Statement& b) -> bool {
                return a.isDescendantOf(b.getStmtNum());
            };

            abs.getAllRelated = [](const Statement& s) -> decltype(auto) {
                return s.getDescendants();
            };

            abs.getAllInverselyRelated = [](const Statement& s) -> decltype(auto) {
                return s.getAncestors();
            };

            abs.relationExists = &ProgramKB::parentRelationExists;
            abs.getEntity = &ProgramKB::getStatementAt;
        }
        abs.evaluate(pkb, tbl, this, &this->ancestor, &this->descendant);
    }
}
