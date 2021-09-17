// evaluator.h
//
// stores declarations for evaluator

#pragma once
#include "pkb.h"
#include "pql/parser/ast.h"
#include "pql/eval/table.h"
#include "simple/ast.h"
#include <list>

namespace pql::eval
{
    class Evaluator
    {
    private:
        pql::eval::table::Table* m_table;
        pkb::ProgramKB* m_pkb;
        pql::ast::Query* m_query;

        // Stores initial domain for all types of declarations
        std::unordered_map<ast::DESIGN_ENT, std::vector<simple::ast::Stmt*>> m_all_ent_stmt_map;

        void preprocessPkb(pkb::ProgramKB* pkb);
        void processDeclarations(const ast::DeclarationList& declaration_list);
        void handleSuchThat(const ast::SuchThatCl* such_that);
        void handlePattern(const ast::PatternCl* pattern);

        void handleFollows(const ast::Follows* follows);
        void handleFollowsT(const ast::FollowsT* follows_t);
        void handleUsesP(const ast::UsesP* uses_p);
        void handleUsesS(const ast::UsesS* uses_p);
        void handleModifiesP(const ast::ModifiesP* modifies_p);
        void handleModifiesS(const ast::ModifiesS* modifies_s);
        void handleParent(const ast::Parent* parent);
        void handleParentT(const ast::ParentT* parent_t);

        std::unordered_set<table::Entry> getInitialDomainVar(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainProc(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainStmt(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomainConst(ast::Declaration* declaration);
        std::unordered_set<table::Entry> getInitialDomain(ast::Declaration* declaration);

    public:
        Evaluator(pkb::ProgramKB* pkb, pql::ast::Query* query);
        std::list<std::string> evaluate();
    };
}
