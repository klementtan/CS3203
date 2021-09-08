
#include <algorithm>
#include "pql/eval/evaluator.h"
#include "pql/eval/table.h"
#include "pql/exception.h"

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
        throw pql::exception::PqlException("pql::eval", "{} does not have a design ent", stmt->toString(1));
    }


    void Evaluator::preprocessPkb(pkb::ProgramKB* pkb)
    {
        for(simple::ast::Stmt* stmt : pkb->statements)
        {
            m_all_ent_stmt_map[getDesignEnt(stmt)].push_back(stmt);
            m_all_ent_stmt_map[ast::DESIGN_ENT::STMT].push_back(stmt);
        }
    }

    std::unordered_set<table::Entry> Evaluator::getInitialDomainVar(ast::Declaration* declaration)
    {
        std::unordered_set<table::Entry> domain;
        if(declaration->design_ent != ast::DESIGN_ENT::VARIABLE)
        {
            throw exception::PqlException(
                "pql::eval", "Cannot get initial domain(var) for non variable declaration {}", declaration->toString());
        }
        for(auto [name, var] : m_pkb->variables)
        {
            domain.insert(table::Entry(declaration, var.name));
        }
        return domain;
    }
    std::unordered_set<table::Entry> Evaluator::getInitialDomainProc(ast::Declaration* declaration)
    {
        std::unordered_set<table::Entry> domain;
        if(declaration->design_ent != ast::DESIGN_ENT::PROCEDURE)
        {
            throw exception::PqlException("pql::eval",
                "Cannot get initial domain(proc) for non variable declaration {}", declaration->toString());
        }
        for(auto [name, proc] : m_pkb->procedures)
        {
            domain.insert(table::Entry(declaration, proc.name));
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
        return getInitialDomainStmt(declaration);
    }

    void Evaluator::processDeclarations(const ast::DeclarationList* declaration_list)
    {
        for(auto [name, decl_ptr] : declaration_list->declarations)
        {
            m_table->upsertDomains(decl_ptr, getInitialDomain(decl_ptr));
        }
    }
    std::list<std::string> Evaluator::evaluate()
    {
        processDeclarations(m_query->declarations);
        if(m_query->select->such_that)
            handleSuchThat(m_query->select->such_that);

        return this->m_table->getResult(m_query->select->ent);
    }
    void Evaluator::handleSuchThat(const ast::SuchThatCl* such_that)
    {
        for(ast::RelCond* rel_cond : such_that->rel_conds)
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
    bool Evaluator::thereExistsFollows(pkb::ProgramKB* pkb)
    {
        for(pkb::Follows* follows : pkb->follows)
        {
            if(follows->after.size() > 0)
                return true;
        }
        return false;
    }
    void Evaluator::handleFollows(const ast::Follows* follows)
    {
        if(dynamic_cast<ast::AllStmt*>(follows->directly_before) &&
            dynamic_cast<ast::AllStmt*>(follows->directly_after) && !thereExistsFollows(m_pkb))
        {
            // There does not exists a pair of following statement
            util::log("pql::eval", "Clearing all domain as {} always evaluates to false", follows->toString());
            m_table->clearDomains();
        }

        if(auto stmt_id_bef = dynamic_cast<ast::StmtId*>(follows->directly_before),
            stmt_id_aft = dynamic_cast<ast::StmtId*>(follows->directly_after);
            stmt_id_bef && stmt_id_aft && !m_pkb->isFollows(stmt_id_bef->id, stmt_id_aft->id))
        {
            // Follows(1,y) always evaluates to false
            util::log("pql::eval", "Clearing all domain as {} always evaluates to false", follows->toString());
            m_table->clearDomains();
        }
        if(auto stmt_id_bef = dynamic_cast<ast::StmtId*>(follows->directly_before))
        {
            if(dynamic_cast<ast::AllStmt*>(follows->directly_after) &&
                m_pkb->getFollows(stmt_id_bef->id)->after.empty())
            {
                // Follows(1,_) always evaluates to false. There does not exist any statement following x
                util::log("pql::eval", "Clearing all domain as {} always evaluates to false", follows->toString());
                m_table->clearDomains();
            }
            if(auto declared_aft = dynamic_cast<ast::DeclaredStmt*>(follows->directly_after); declared_aft)
            {
                // Update x to fulfill Follows(1, x)
                simple::ast::StatementNum expected_statement_num = m_pkb->getFollows(stmt_id_bef->id)->directly_after;
                auto entry = pql::eval::table::Entry(declared_aft->declaration, expected_statement_num);
                util::log("pql::eval", "{} restricts domain of {} to [{}]", follows->toString(),
                    declared_aft->declaration->toString(), entry.toString());
                std::unordered_set<pql::eval::table::Entry> curr_domain = { entry };
                std::unordered_set<pql::eval::table::Entry> prev_domain = m_table->getDomain(declared_aft->declaration);
                std::unordered_set<pql::eval::table::Entry> new_domain =
                    table::entry_set_intersect(curr_domain, prev_domain);
                m_table->upsertDomains(declared_aft->declaration, new_domain);
            }
        }
        if(auto stmt_id_aft = dynamic_cast<ast::StmtId*>(follows->directly_after))
        {
            if(dynamic_cast<ast::AllStmt*>(follows->directly_before) &&
                m_pkb->getFollows(stmt_id_aft->id)->before.empty())
            {
                // Follows(_,1) always evaluates to false. There does not exist any statement that x follows
                util::log("pql::eval", "Clearing all domain as {} always evaluates to false", follows->toString());
                m_table->clearDomains();
            }
            if(auto declared_bef = dynamic_cast<ast::DeclaredStmt*>(follows->directly_before); declared_bef)
            {
                // Update x to fulfill Follows(1, x)
                simple::ast::StatementNum expected_statement_num = m_pkb->getFollows(stmt_id_aft->id)->directly_before;
                auto entry = pql::eval::table::Entry(declared_bef->declaration, expected_statement_num);
                util::log("pql::eval", "{} restricts domain of {} to [{}]", follows->toString(),
                    declared_bef->declaration->toString(), entry.toString());
                std::unordered_set<pql::eval::table::Entry> curr_domain = { entry };
                std::unordered_set<pql::eval::table::Entry> prev_domain = m_table->getDomain(declared_bef->declaration);
                std::unordered_set<pql::eval::table::Entry> new_domain =
                    table::entry_set_intersect(curr_domain, prev_domain);
                m_table->upsertDomains(declared_bef->declaration, new_domain);
            }
        }

        ast::DeclaredStmt* directly_bef = dynamic_cast<ast::DeclaredStmt*>(follows->directly_before);
        ast::DeclaredStmt* directly_aft = dynamic_cast<ast::DeclaredStmt*>(follows->directly_after);

        // Should only left with (declared_stmt, declared_stmt)
        assert(directly_aft && directly_bef);

        for(const table::Entry& entry : m_table->getDomain(directly_bef->declaration))
        {
            pkb::Follows* bef_follows = m_pkb->getFollows(entry.getStmtNum());
            table::Entry bef_entry = table::Entry(directly_bef->declaration, bef_follows->id);
            table::Entry aft_entry = table::Entry(directly_aft->declaration, bef_follows->id);
            util::log("pql::eval", "Adding Join({},{}) for {}", bef_entry.toString(), aft_entry.toString(),
                follows->toString());
            m_table->addJoin(bef_entry, aft_entry);
        }
    }
    void Evaluator::handleFollowsT(const ast::FollowsT* follows_t) { }
    void Evaluator::handleUsesP(const ast::UsesP* uses_p) { }
    void Evaluator::handleUsesS(const ast::UsesS* uses_p) { }
    void Evaluator::handleModifiesP(const ast::ModifiesP* modifies_p) { }
    void Evaluator::handleModifiesS(const ast::ModifiesS* modifies_s) { }
    void Evaluator::handleParent(const ast::Parent* parent) { }
    void Evaluator::handleParentT(const ast::ParentT* parent_t) { }
}
