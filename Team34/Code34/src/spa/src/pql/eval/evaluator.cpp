// evaluator.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    Evaluator::Evaluator(pkb::ProgramKB* pkb, std::unique_ptr<ast::Query> query)
    {
        this->m_pkb = pkb;
        this->m_query = std::move(query);

        preprocessPkb();
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


    void Evaluator::preprocessPkb()
    {
        for(const auto& pkb_stmt : m_pkb->getAllStatements())
        {
            m_all_ent_stmt_map[getDesignEnt(pkb_stmt.getAstStmt())].push_back(pkb_stmt.getAstStmt());
            m_all_ent_stmt_map[ast::DESIGN_ENT::STMT].push_back(pkb_stmt.getAstStmt());
        }
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
        util::log("pql::eval", "Adding {} variables to {} initial domain", var_list.size(), declaration->toString());
        for(const auto& [name, var] : var_list)
        {
            util::log("pql::eval", "Adding {} to initial var domain", name);
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
        util::log("pql::eval", "Adding {} procedures to {} initial domain", proc_list.size(), declaration->toString());
        for(const auto& [name, proc] : proc_list)
        {
            util::log("pql::eval", "Adding {} to initial proc domain", name);
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
        util::log(
            "pql::eval", "Adding {} constants to {} initial domain", const_list.size(), declaration->toString());
        for(const auto& const_val : const_list)
        {
            util::log("pql::eval", "Adding {} to initial proc domain", const_val);
            domain.insert(table::Entry(declaration, const_val));
        }
        return domain;
    }
    std::unordered_set<table::Entry> Evaluator::getInitialDomainStmt(ast::Declaration* declaration)
    {
        std::unordered_set<table::Entry> domain;
        auto it = m_all_ent_stmt_map.find(declaration->design_ent);
        if(it == m_all_ent_stmt_map.end())
        {
            util::log("pql::eval", "No statement in source for {}", declaration->toString());
            return domain;
        }
        for(const simple::ast::Stmt* stmt : it->second)
        {
            util::log("pql::eval", "Adding {} to initial stmt domain", stmt->toString(0));
            domain.insert(table::Entry(declaration, stmt->id));
        }
        return domain;
    }

    std::unordered_set<table::Entry> Evaluator::getInitialDomain(ast::Declaration* declaration)
    {
        util::log("pql::eval", "Getting initial domain for {}", declaration->toString());
        if(declaration->design_ent == ast::DESIGN_ENT::VARIABLE)
            return getInitialDomainVar(declaration);
        if(declaration->design_ent == ast::DESIGN_ENT::PROCEDURE)
            return getInitialDomainProc(declaration);
        if(declaration->design_ent == ast::DESIGN_ENT::CONSTANT)
            return getInitialDomainConst(declaration);
        return getInitialDomainStmt(declaration);
    }

    void Evaluator::processDeclarations(const ast::DeclarationList& declaration_list)
    {
        for(const auto& [_, decl_ptr] : declaration_list.getAllDeclarations())
            m_table.upsertDomains(decl_ptr, getInitialDomain(decl_ptr));
    }

    std::list<std::string> Evaluator::evaluate()
    {
        util::log("pql::eval", "Evaluating query: {}", m_query->toString());
        processDeclarations(m_query->declarations);
        util::log("pql::eval", "Table after initial processing of declaration: {}", m_table.toString());

        // All queries should have select clause
        assert(m_query->select.ent);
        m_table.addSelectDecl(m_query->select.ent);

        if(m_query->select.such_that)
            handleSuchThat(*m_query->select.such_that);

        if(m_query->select.pattern)
            this->handlePattern(*m_query->select.pattern);

        util::log("pql::eval", "Table after processing of such that: {}", m_table.toString());
        return this->m_table.getResult(m_query->select.ent);
    }

    void Evaluator::handleSuchThat(const ast::SuchThatCl& such_that)
    {
        util::log("pql::eval", "Handling such that:{}", such_that.toString());
        for(const auto& rel_cond : such_that.rel_conds)
        {
            if(auto follows = dynamic_cast<ast::Follows*>(rel_cond.get()); follows)
                handleFollows(follows);
            else if(auto follows_t = dynamic_cast<ast::FollowsT*>(rel_cond.get()); follows_t)
                handleFollowsT(follows_t);
            else if(auto uses_p = dynamic_cast<ast::UsesP*>(rel_cond.get()); uses_p)
                handleUsesP(uses_p);
            else if(auto uses_s = dynamic_cast<ast::UsesS*>(rel_cond.get()); uses_s)
                handleUsesS(uses_s);
            else if(auto modifies_p = dynamic_cast<ast::ModifiesP*>(rel_cond.get()); modifies_p)
                handleModifiesP(modifies_p);
            else if(auto modifies_s = dynamic_cast<ast::ModifiesS*>(rel_cond.get()); modifies_s)
                handleModifiesS(modifies_s);
            else if(auto parent = dynamic_cast<ast::Parent*>(rel_cond.get()); parent)
                handleParent(parent);
            else if(auto parent_t = dynamic_cast<ast::ParentT*>(rel_cond.get()); parent_t)
                handleParentT(parent_t);
            else
                throw util::PqlException("pql::eval", "unknown relation type");
        }
    }
}
