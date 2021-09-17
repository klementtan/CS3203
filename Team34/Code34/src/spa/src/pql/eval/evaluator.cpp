// evaluator.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    Evaluator::Evaluator(pkb::ProgramKB* pkb, pql::ast::Query* query)
    {
        this->m_query = query;
        this->m_pkb = pkb;
        this->m_table = new pql::eval::table::Table();
        preprocessPkb(this->m_pkb);
    }

    ast::DESIGN_ENT getDesignEnt(simple::ast::Stmt* stmt)
    {
        if(dynamic_cast<simple::ast::AssignStmt*>(stmt))
            return ast::DESIGN_ENT::ASSIGN;
        if(dynamic_cast<simple::ast::IfStmt*>(stmt))
            return ast::DESIGN_ENT::IF;
        if(dynamic_cast<simple::ast::PrintStmt*>(stmt))
            return ast::DESIGN_ENT::PRINT;
        if(dynamic_cast<simple::ast::ReadStmt*>(stmt))
            return ast::DESIGN_ENT::READ;
        if(dynamic_cast<simple::ast::WhileLoop*>(stmt))
            return ast::DESIGN_ENT::WHILE;
        if(dynamic_cast<simple::ast::ProcCall*>(stmt))
            return ast::DESIGN_ENT::CALL;
        throw util::PqlException("pql::eval", "{} does not have a design ent", stmt->toString(1));
    }


    void Evaluator::preprocessPkb(pkb::ProgramKB* pkb)
    {
        for(pkb::Statement* pkb_stmt : pkb->uses_modifies.statements)
        {
            m_all_ent_stmt_map[getDesignEnt(pkb_stmt->stmt)].push_back(pkb_stmt->stmt);
            m_all_ent_stmt_map[ast::DESIGN_ENT::STMT].push_back(pkb_stmt->stmt);
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
        util::log("pql::eval", "Adding {} variables to {} initial domain", m_pkb->uses_modifies.variables.size(),
            declaration->toString());
        for(auto [name, var] : m_pkb->uses_modifies.variables)
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
        util::log("pql::eval", "Adding {} procedures to {} initial domain", m_pkb->uses_modifies.procedures.size(),
            declaration->toString());
        for(auto [name, proc] : m_pkb->uses_modifies.procedures)
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
        util::log("pql::eval", "Adding {} constants to {} initial domain", m_pkb->uses_modifies.procedures.size(),
            declaration->toString());
        for(const auto& const_val : m_pkb->getConstants())
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
        for(simple::ast::Stmt* stmt : it->second)
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
            m_table->upsertDomains(decl_ptr, getInitialDomain(decl_ptr));
    }

    std::list<std::string> Evaluator::evaluate()
    {
        util::log("pql::eval", "Evaluating query: {}", m_query->toString());
        processDeclarations(m_query->declarations);
        util::log("pql::eval", "Table after initial processing of declaration: {}", m_table->toString());

        // All queries should have select clause
        assert(m_query->select.ent);
        m_table->addSelectDecl(m_query->select.ent);

        if(m_query->select.such_that)
            handleSuchThat(*m_query->select.such_that);

        if(m_query->select.pattern)
            this->handlePattern(*m_query->select.pattern);

        util::log("pql::eval", "Table after processing of such that: {}", m_table->toString());
        return this->m_table->getResult(m_query->select.ent);
    }

    void Evaluator::handleSuchThat(const ast::SuchThatCl& such_that)
    {
        util::log("pql::eval", "Handling such that:{}", such_that.toString());
        for(ast::RelCond* rel_cond : such_that.rel_conds)
        {
            if(auto follows = dynamic_cast<ast::Follows*>(rel_cond); follows)
                handleFollows(follows);
            if(auto follows_t = dynamic_cast<ast::FollowsT*>(rel_cond); follows_t)
                handleFollowsT(follows_t);
            if(auto uses_p = dynamic_cast<ast::UsesP*>(rel_cond); uses_p)
                handleUsesP(uses_p);
            if(auto uses_s = dynamic_cast<ast::UsesS*>(rel_cond); uses_s)
                handleUsesS(uses_s);
            if(auto modifies_p = dynamic_cast<ast::ModifiesP*>(rel_cond); modifies_p)
                handleModifiesP(modifies_p);
            if(auto modifies_s = dynamic_cast<ast::ModifiesS*>(rel_cond); modifies_s)
                handleModifiesS(modifies_s);
            if(auto parent = dynamic_cast<ast::Parent*>(rel_cond); parent)
                handleParent(parent);
            if(auto parent_t = dynamic_cast<ast::ParentT*>(rel_cond); parent_t)
                handleParentT(parent_t);
        }
    }
}
