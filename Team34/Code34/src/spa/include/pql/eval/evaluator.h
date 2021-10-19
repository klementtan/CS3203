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

        void processDeclarations(const ast::DeclarationList& declaration_list);

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
