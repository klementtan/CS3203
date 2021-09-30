// parent.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using namespace pkb;
    using PqlException = util::PqlException;
    using StatementSet = std::unordered_set<StatementNum>;

    void Evaluator::handleParent(const ast::Parent* rel)
    {
        assert(rel);

        RelationAbstractor<Statement, StatementNum, ast::StmtRef, /* SetsAreConstRef: */ true> abs {};
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

        abs.relationExists = &pkb::ProgramKB::parentRelationExists;
        abs.getEntity = &pkb::ProgramKB::getStatementAt;
        abs.getEntryValue = &table::Entry::getStmtNum;

        abs.evaluate(m_pkb, &m_table, rel, &rel->parent, &rel->child);
    }




    void Evaluator::handleParentT(const ast::ParentT* rel)
    {
        assert(rel);

        RelationAbstractor<Statement, StatementNum, ast::StmtRef, /* SetsAreConstRef: */ true> abs {};
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

        abs.relationExists = &pkb::ProgramKB::parentRelationExists;
        abs.getEntity = &pkb::ProgramKB::getStatementAt;
        abs.getEntryValue = &table::Entry::getStmtNum;

        abs.evaluate(m_pkb, &m_table, rel, &rel->ancestor, &rel->descendant);
    }
}
