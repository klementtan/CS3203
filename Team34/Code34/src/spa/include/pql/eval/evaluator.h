// evaluator.h
//
// stores declarations for evaluator

#pragma once

#include <list>
#include <memory>
#include <functional>

#include "pkb.h"
#include "pql/parser/ast.h"
#include "pql/eval/table.h"
#include "simple/ast.h"

namespace pql::eval
{
    ast::DESIGN_ENT getDesignEnt(const simple::ast::Stmt* stmt);

    class Evaluator
    {
    private:
        const pkb::ProgramKB* m_pkb;

        table::Table m_table;
        std::unique_ptr<ast::Query> m_query;

        // Stores initial domain for all types of declarations
        std::unordered_map<ast::DESIGN_ENT, std::vector<const simple::ast::Stmt*>> m_all_ent_stmt_map;

        void preprocessPkb();
        void processDeclarations(const ast::DeclarationList& declaration_list);
        void handleSuchThat(const ast::SuchThatCl& such_that);
        void handlePattern(const ast::PatternCl& pattern);

        void handleFollows(const ast::Follows* follows);
        void handleFollowsT(const ast::FollowsT* follows_t);
        void handleUsesP(const ast::UsesP* uses_p);
        void handleUsesS(const ast::UsesS* uses_p);
        void handleModifiesP(const ast::ModifiesP* modifies_p);
        void handleModifiesS(const ast::ModifiesS* modifies_s);
        void handleParent(const ast::Parent* parent);
        void handleParentT(const ast::ParentT* parent_t);
        void handleCalls(const ast::Calls* rel);
        void handleCallsT(const ast::CallsT* rel);

        std::unordered_set<table::Entry> getInitialDomainVar(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainProc(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainStmt(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainConst(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomain(ast::Declaration* declaration);

    public:
        Evaluator(const pkb::ProgramKB* pkb, std::unique_ptr<ast::Query> query);
        std::list<std::string> evaluate();
    };




    template <typename Entity, typename RelationParam, typename RefType, bool SetsAreConstRef>
    struct RelationAbstractor
    {
        const char* relationName = nullptr;

        // what kind of entity must the declaration be, if the left/right refs are indeed decls.
        // optional; if empty, then this is not enforced.
        std::optional<ast::DESIGN_ENT> leftDeclEntity {};
        std::optional<ast::DESIGN_ENT> rightDeclEntity {};

        // Relation[*](A, B) <=> A.relationHolds(B) <=> B.inverseRelationHolds(A)
        std::function<bool(const Entity&, const Entity&)> relationHolds {};
        std::function<bool(const Entity&, const Entity&)> inverseRelationHolds {};

        template <typename T>
        using SetWrapper = std::conditional_t<SetsAreConstRef, const std::unordered_set<T>&, std::unordered_set<T>>;

        // Relation[*](A, _) <=> A.getAllRelated()
        // Relation[*](_, B) <=> B.getAllInverselyRelated()
        std::function<SetWrapper<RelationParam>(const Entity&)> getAllRelated {};
        std::function<SetWrapper<RelationParam>(const Entity&)> getAllInverselyRelated {};

        // getStatementAt, getProcedureNamed, getVariableNamed
        const Entity& (pkb::ProgramKB::*getEntity)(const RelationParam&) const;

        // Entry::getVal(), getStmtNum()
        RelationParam (table::Entry::*getEntryValue)() const;

        // callsRelationExists, parentRelationExists, etc.
        bool (pkb::ProgramKB::*relationExists)() const;

        void evaluate(const pkb::ProgramKB* pkb, table::Table* table, const ast::RelCond* rel, const RefType* left,
            const RefType* right);
    };
}
