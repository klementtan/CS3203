// evaluator.h
//
// stores declarations for evaluator

#pragma once

#include <list>
#include <memory>

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
        void handleRelation(const ast::RelCond* rel_cond);

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
        void handleNext(const ast::Next* rel);
        void handleNextT(const ast::NextT* rel);

        std::unordered_set<table::Entry> getInitialDomainVar(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainProc(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainStmt(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainConst(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomain(ast::Declaration* declaration);

    public:
        Evaluator(const pkb::ProgramKB* pkb, std::unique_ptr<ast::Query> query);
        std::list<std::string> evaluate();
    };
}
