// evaluator.cpp

#include <algorithm>

#include "timer.h"
#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    Evaluator::Evaluator(const pkb::ProgramKB* pkb, std::unique_ptr<ast::Query> query)
    {
        this->m_pkb = pkb;
        this->m_query = std::move(query);
    }

    ast::DESIGN_ENT getDesignEnt(const simple::ast::Stmt* stmt)
    {
        if(dynamic_cast<const simple::ast::AssignStmt*>(stmt))
            return ast::DESIGN_ENT::ASSIGN;
        if(dynamic_cast<const simple::ast::IfStmt*>(stmt))
            return ast::DESIGN_ENT::IF;
        if(dynamic_cast<const simple::ast::PrintStmt*>(stmt))
            return ast::DESIGN_ENT::PRINT;
        if(dynamic_cast<const simple::ast::ReadStmt*>(stmt))
            return ast::DESIGN_ENT::READ;
        if(dynamic_cast<const simple::ast::WhileLoop*>(stmt))
            return ast::DESIGN_ENT::WHILE;
        if(dynamic_cast<const simple::ast::ProcCall*>(stmt))
            return ast::DESIGN_ENT::CALL;
        throw util::PqlException("pql::eval", "{} does not have a design ent", stmt->toString(1));
    }

    std::unordered_set<table::Entry> Evaluator::getInitialDomainVar(ast::Declaration* declaration)
    {
        std::unordered_set<table::Entry> domain;
        if(declaration->design_ent != ast::DESIGN_ENT::VARIABLE)
        {
            throw util::PqlException(
                "pql::eval", "Cannot get initial domain(var) for non variable declaration {}", declaration->toString());
        }

        auto& var_list = m_pkb->getAllVariables();
        util::logfmt("pql::eval", "Adding {} variables to {} initial domain", var_list.size(), declaration->toString());
        for(const auto& [name, var] : var_list)
        {
            util::logfmt("pql::eval", "Adding {} to initial var domain", name);
            domain.insert(table::Entry(declaration, name));
        }
        return domain;
    }
    std::unordered_set<table::Entry> Evaluator::getInitialDomainProc(ast::Declaration* declaration)
    {
        std::unordered_set<table::Entry> domain;
        if(declaration->design_ent != ast::DESIGN_ENT::PROCEDURE)
        {
            throw util::PqlException("pql::eval", "Cannot get initial domain(proc) for non variable declaration {}",
                declaration->toString());
        }

        auto& proc_list = m_pkb->getAllProcedures();
        util::logfmt(
            "pql::eval", "Adding {} procedures to {} initial domain", proc_list.size(), declaration->toString());
        for(const auto& [name, proc] : proc_list)
        {
            util::logfmt("pql::eval", "Adding {} to initial proc domain", name);
            domain.insert(table::Entry(declaration, name));
        }
        return domain;
    }
    std::unordered_set<table::Entry> Evaluator::getInitialDomainConst(ast::Declaration* declaration)
    {
        std::unordered_set<table::Entry> domain;
        if(declaration->design_ent != ast::DESIGN_ENT::CONSTANT)
        {
            throw util::PqlException("pql::eval", "Cannot get initial domain(constant) for non constant declaration {}",
                declaration->toString());
        }
        auto& const_list = m_pkb->getAllConstants();
        util::logfmt(
            "pql::eval", "Adding {} constants to {} initial domain", const_list.size(), declaration->toString());
        for(const auto& const_val : const_list)
        {
            util::logfmt("pql::eval", "Adding {} to initial proc domain", const_val);
            domain.insert(table::Entry(declaration, const_val));
        }
        return domain;
    }
    std::unordered_set<table::Entry> Evaluator::getInitialDomainStmt(ast::Declaration* declaration)
    {
        std::unordered_set<table::Entry> domain {};
        const auto& all_stmts = m_pkb->getAllStatementsOfKind(declaration->design_ent);
        for(auto sid : all_stmts)
            domain.emplace(declaration, sid);

        return domain;
    }

    std::unordered_set<table::Entry> Evaluator::getInitialDomain(ast::Declaration* declaration)
    {
        util::logfmt("pql::eval", "Getting initial domain for {}", declaration->toString());

        if(declaration->design_ent == ast::DESIGN_ENT::VARIABLE)
            return getInitialDomainVar(declaration);

        else if(declaration->design_ent == ast::DESIGN_ENT::PROCEDURE)
            return getInitialDomainProc(declaration);

        else if(declaration->design_ent == ast::DESIGN_ENT::CONSTANT)
            return getInitialDomainConst(declaration);

        else
            return getInitialDomainStmt(declaration);
    }

    void Evaluator::processDeclarations(const ast::DeclarationList& declaration_list)
    {
        for(const auto& [_, decl_ptr] : declaration_list.getAllDeclarations())
            m_table.putDomain(decl_ptr, getInitialDomain(decl_ptr));
    }

    std::list<std::string> Evaluator::evaluate()
    {
        START_BENCHMARK_TIMER("PQL Evaluation Timer");
        util::logfmt("pql::eval", "Evaluating query: {}", m_query->toString());

        if(m_query->isInvalid())
        {
            util::logfmt("pql::eval", "refusing to evaluate; query was semantically invalid");
            return table::Table::getFailedResult(m_query->select.result);
        }

        // this should check for exceptions.
        try
        {
            processDeclarations(m_query->declarations);
            util::logfmt("pql::eval", "Table after initial processing of declaration: {}", m_table.toString());

            {
                START_BENCHMARK_TIMER("Evaluate all clauses");
                for(const auto& clause : m_query->select.clauses)
                    clause->evaluate(m_pkb, &m_table);
            }

            util::logfmt("pql::eval", "Table after processing of such that: {}", m_table.toString());
            return this->m_table.getResult(m_query->select.result, this->m_pkb);
        }
        catch(const util::Exception& e)
        {
            util::logfmt("pql::eval", "caught exception during evaluation of query: '{}'", e.what());
            return table::Table::getFailedResult(m_query->select.result);
        }
    }
}
