
#include <algorithm>
#include "pql/eval/evaluator.h"
#include <cassert>
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
            throw exception::PqlException(
                "pql::eval", "Cannot get initial domain(var) for non variable declaration {}", declaration->toString());
        }
        util::log("pql::eval", "Adding {} variables to {} initial domain", m_pkb->uses_modifies.variables.size(),
            declaration->toString());
        // TODO: Check with pkb team if this is the right way to access all variables
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
            throw exception::PqlException("pql::eval",
                "Cannot get initial domain(proc) for non variable declaration {}", declaration->toString());
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
        util::log("pql::eval", "Evaluating query: {}", m_query->toString());
        processDeclarations(m_query->declarations);
        util::log("pql::eval", "Table after initial processing of declaration: {}", m_table->toString());
        if(m_query->select->such_that)
            handleSuchThat(m_query->select->such_that);
        util::log("pql::eval", "Table after processing of such that: {}", m_table->toString());
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
        return std::any_of(
            pkb->follows.begin(), pkb->follows.end(), [](pkb::Follows* follows) { return !follows->after.empty(); });
    }
    void Evaluator::handleFollows(const ast::Follows* follows)
    {
        assert(follows);
        assert(follows->directly_after);
        assert(follows->directly_before);
        auto* bef_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_before);
        auto* aft_stmt_id = dynamic_cast<ast::StmtId*>(follows->directly_after);
        auto* bef_all = dynamic_cast<ast::AllStmt*>(follows->directly_before);
        auto* aft_all = dynamic_cast<ast::AllStmt*>(follows->directly_after);
        auto* bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_before);
        auto* aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows->directly_after);

        if(bef_stmt_id && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows(StmtId,StmtId)");
            if(m_pkb->isFollows(bef_stmt_id->id, aft_stmt_id->id))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
            else
            {
                throw pql::exception::PqlException(
                    "pql::eval", "{} will always evaluate to false", follows->toString());
            }
        }
        if(bef_stmt_id && aft_all)
        {
            util::log("pql::eval", "Processing Follows(StmtId,_)");
            if(m_pkb->getFollows(bef_stmt_id->id)->after.empty())
            {
                throw pql::exception::PqlException(
                    "pql::eval", "{} will always evaluate to false", follows->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
        }
        if(bef_stmt_id && aft_decl)
        {
            util::log("pql::eval", "Processing Follows(StmtId,DeclaredStmt)");
            if(m_pkb->getFollows(bef_stmt_id->id)->after.empty())
            {
                throw pql::exception::PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly after {}", follows->toString(),
                    bef_stmt_id->toString());
            }
            else
            {
                table::Entry entry =
                    table::Entry(aft_decl->declaration, m_pkb->getFollows(bef_stmt_id->id)->directly_after);
                util::log("pql::eval", "{} updating domain to [{}]", follows->toString(), entry.toString());
                std::unordered_set<table::Entry> curr_domain = { entry };
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl->declaration);
                m_table->upsertDomains(aft_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(bef_all && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows(_,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id->id)->before.empty())
            {
                throw pql::exception::PqlException(
                    "pql::eval", "{} will always evaluate to false", follows->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
        }
        if(bef_all && aft_all)
        {
            util::log("pql::eval", "Processing Follows(_,_)");
            if(thereExistsFollows(this->m_pkb))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows->toString());
                return;
            }
            else
            {
                throw exception::PqlException(
                    "pql::eval", "{} will always evaluate to false. No 2 following statement.", follows->toString());
            }
        }
        if(bef_all && aft_decl)
        {
            util::log("pql::eval", "Processing Follows(_,DeclaredStmt)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl->declaration);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->before.empty())
                {
                    table::Entry entry = table::Entry(aft_decl->declaration, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(aft_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(bef_decl && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows(DeclaredStmt,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id->id)->before.empty())
            {
                throw pql::exception::PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly directly before {}", follows->toString(),
                    aft_stmt_id->toString());
            }
            else
            {
                auto entry = table::Entry(bef_decl->declaration, m_pkb->getFollows(aft_stmt_id->id)->directly_before);
                util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                std::unordered_set<table::Entry> curr_domain = { entry };
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl->declaration);
                m_table->upsertDomains(bef_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(bef_decl && aft_all)
        {
            util::log("pql::eval", "Processing Follows(DeclaredStmt,_)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl->declaration);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->after.empty())
                {
                    table::Entry entry = table::Entry(bef_decl->declaration, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(bef_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(bef_decl && aft_decl)
        {
            util::log("pql::eval", "Processing Follows(DeclaredStmt,DeclaredStmt)");
            for(const table::Entry& entry : m_table->getDomain(bef_decl->declaration))
            {
                pkb::Follows* bef_follows = m_pkb->getFollows(entry.getStmtNum());
                if(bef_follows->after.empty())
                    continue;
                table::Entry bef_entry = table::Entry(bef_decl->declaration, bef_follows->id);
                table::Entry aft_entry = table::Entry(aft_decl->declaration, bef_follows->directly_after);
                util::log("pql::eval", "{} adds Join({},{})", follows->toString(), bef_entry.toString(),
                    aft_entry.toString());
                m_table->addJoin(bef_entry, aft_entry);
            }
        }
    }
    void Evaluator::handleFollowsT(const ast::FollowsT* follows_t)
    {
        assert(follows_t);
        assert(follows_t->after);
        assert(follows_t->before);
        auto* bef_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->before);
        auto* aft_stmt_id = dynamic_cast<ast::StmtId*>(follows_t->after);
        auto* bef_all = dynamic_cast<ast::AllStmt*>(follows_t->before);
        auto* aft_all = dynamic_cast<ast::AllStmt*>(follows_t->after);
        auto* bef_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->before);
        auto* aft_decl = dynamic_cast<ast::DeclaredStmt*>(follows_t->after);

        if(bef_stmt_id && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows*(StmtId,StmtId)");
            if(m_pkb->isFollowsT(bef_stmt_id->id, aft_stmt_id->id))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
            else
            {
                throw pql::exception::PqlException(
                    "pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
        }
        if(bef_stmt_id && aft_all)
        {
            util::log("pql::eval", "Processing Follows*(StmtId,_)");
            if(m_pkb->getFollows(bef_stmt_id->id)->after.empty())
            {
                throw pql::exception::PqlException(
                    "pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
        }
        if(bef_stmt_id && aft_decl)
        {
            util::log("pql::eval", "Processing Follows*(StmtId,DeclaredStmt)");
            if(m_pkb->getFollows(bef_stmt_id->id)->after.empty())
            {
                throw pql::exception::PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly after {}", follows_t->toString(),
                    bef_stmt_id->toString());
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                for(simple::ast::StatementNum after_stmt_num : m_pkb->getFollows(bef_stmt_id->id)->after)
                {
                    table::Entry entry = table::Entry(aft_decl->declaration, after_stmt_num);
                    util::log("pql::eval", "{} updating domain to [{}]", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl->declaration);
                m_table->upsertDomains(aft_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(bef_all && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows*(_,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id->id)->before.empty())
            {
                throw pql::exception::PqlException(
                    "pql::eval", "{} will always evaluate to false", follows_t->toString());
            }
            else
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
        }
        if(bef_all && aft_all)
        {
            util::log("pql::eval", "Processing Follows*(_,_)");
            if(thereExistsFollows(this->m_pkb))
            {
                util::log("pql::eval", "{} will always evaluate to true", follows_t->toString());
                return;
            }
            else
            {
                throw exception::PqlException(
                    "pql::eval", "{} will always evaluate to false. No 2 following statement.", follows_t->toString());
            }
        }
        if(bef_all && aft_decl)
        {
            util::log("pql::eval", "Processing Follows*(_,DeclaredStmt)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(aft_decl->declaration);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->before.empty())
                {
                    table::Entry entry = table::Entry(aft_decl->declaration, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(aft_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(bef_decl && aft_stmt_id)
        {
            util::log("pql::eval", "Processing Follows*(DeclaredStmt,StmtId)");
            if(m_pkb->getFollows(aft_stmt_id->id)->before.empty())
            {
                throw pql::exception::PqlException("pql::eval",
                    "{} will always evaluate to false. No statement directly directly before {}", follows_t->toString(),
                    aft_stmt_id->toString());
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl->declaration);
                for(simple::ast::StatementNum bef_stmt_id : m_pkb->getFollows(aft_stmt_id->id)->before)
                {
                    auto entry = table::Entry(bef_decl->declaration, bef_stmt_id);
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                }
                m_table->upsertDomains(bef_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(bef_decl && aft_all)
        {
            util::log("pql::eval", "Processing Follows*(DeclaredStmt,_)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(bef_decl->declaration);
            for(auto pkb_follows : m_pkb->follows)
            {
                // Add all stmts with a directly before to domain
                if(!pkb_follows->after.empty())
                {
                    table::Entry entry = table::Entry(bef_decl->declaration, pkb_follows->id);
                    util::log("pql::eval", "{} adds {} to curr domain", follows_t->toString(), entry.toString());
                    curr_domain.insert(entry);
                }
            }
            m_table->upsertDomains(bef_decl->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(bef_decl && aft_decl)
        {
            util::log("pql::eval", "Processing Follows*(DeclaredStmt,DeclaredStmt)");
            for(const table::Entry& entry : m_table->getDomain(bef_decl->declaration))
            {
                pkb::Follows* bef_follows = m_pkb->getFollows(entry.getStmtNum());
                if(bef_follows->after.empty())
                    continue;
                for(simple::ast::StatementNum after_stmt_num : bef_follows->after)
                {
                    table::Entry bef_entry = table::Entry(bef_decl->declaration, bef_follows->id);
                    table::Entry aft_entry = table::Entry(aft_decl->declaration, after_stmt_num);
                    util::log("pql::eval", "{} adds Join({},{})", follows_t->toString(), bef_entry.toString(),
                        aft_entry.toString());
                    m_table->addJoin(bef_entry, aft_entry);
                }
            }
        }
    }
    void Evaluator::handleUsesP(const ast::UsesP* uses_p) { }
    void Evaluator::handleUsesS(const ast::UsesS* uses_p) { }

    void Evaluator::handleModifiesP(const ast::ModifiesP* modifies_p)
    {
        assert(modifies_p);
        assert(modifies_p->ent);
        assert(modifies_p->modifier);
        auto* mod_declared = dynamic_cast<ast::DeclaredEnt*>(modifies_p->modifier);
        auto* mod_name = dynamic_cast<ast::EntName*>(modifies_p->modifier);
        auto* mod_all = dynamic_cast<ast::AllEnt*>(modifies_p->modifier);
        auto* ent_declared = dynamic_cast<ast::DeclaredEnt*>(modifies_p->ent);
        auto* ent_name = dynamic_cast<ast::EntName*>(modifies_p->ent);
        auto* ent_all = dynamic_cast<ast::AllEnt*>(modifies_p->ent);
        if(mod_all)
        {
            throw exception::PqlException(
                "pql::eval", "Modifier of ModifiesP cannot be '_': {}", modifies_p->toString());
        }
        if(mod_declared && (mod_declared->declaration->design_ent != ast::DESIGN_ENT::PROCEDURE))
        {
            throw exception::PqlException("pql::eval",
                "Declared modifier of ModifiesP can only be of type PROCEDURE: {}", modifies_p->toString());
        }
        if(ent_declared && (ent_declared->declaration->design_ent != ast::DESIGN_ENT::VARIABLE))
        {
            throw exception::PqlException(
                "pql::eval", "Entity being modified must be of type VARIABLE: {}", modifies_p->toString());
        }
        if(mod_declared && ent_declared)
        {
            util::log("pql::eval", "Processing ModifiesP(DeclaredEnt, DeclaredEnt)");
            for(const table::Entry& entry : m_table->getDomain(mod_declared->declaration))
            {
                std::unordered_set<std::string> ent_names = this->m_pkb->uses_modifies.getModifiesVars(entry.getVal());
                for(std::string ent_name : ent_names)
                {
                    table::Entry mod_entry = table::Entry(mod_declared->declaration, entry.getVal());
                    table::Entry ent_entry = table::Entry(ent_declared->declaration, ent_name);
                    util::log("pql::eval", "{} adds Join({},{}),", modifies_p->toString(), mod_entry.toString(),
                        ent_entry.toString());
                    m_table->addJoin(mod_entry, ent_entry);
                }
            }
        }
        if(mod_declared && ent_name)
        {
            util::log("pql::eval", "Processing ModifiesP(DeclaredEnt, EntName)");

            std::unordered_set<std::string> modifier_candidates =
                m_pkb->uses_modifies.getModifies(mod_declared->declaration->design_ent, ent_name->name);
            if(modifier_candidates.empty())
            {
                throw pql::exception::PqlException("pql::eval",
                    "{} will always evaluate to false. No procedures modifies {}", modifies_p->toString(),
                    ent_name->name);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(mod_declared->declaration);
                for(std::string proc_name : modifier_candidates)
                {
                    auto entry = table::Entry(mod_declared->declaration, proc_name);
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", modifies_p->toString(), entry.toString());
                }
                m_table->upsertDomains(mod_declared->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(mod_declared && ent_all)
        {
            util::log("pql::eval", "Processing ModifiesP(DeclaredEnt, _)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(mod_declared->declaration);
            for(const table::Entry& entry : m_table->getDomain(mod_declared->declaration))
            {
                std::string proc_name = entry.getVal();
                if(!m_pkb->uses_modifies.getModifiesVars(proc_name).empty())
                {
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", modifies_p->toString(), entry.toString());
                }
            }
            m_table->upsertDomains(mod_declared->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(mod_name && ent_declared)
        {
            util::log("pql::eval", "Processing ModifiesP(EntName, DeclaredEnt)");
            std::unordered_set<std::string> var_candidates = m_pkb->uses_modifies.getModifiesVars(mod_name->name);
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(ent_declared->declaration);
            for(const std::string& var : var_candidates)
            {
                util::log("pql::eval", "Adding {} modifies {}", mod_name->name, var);
                auto entry = table::Entry(ent_declared->declaration, var);
                curr_domain.insert(entry);
                util::log("pql::eval", "{} adds {} to curr domain", modifies_p->toString(), entry.toString());
            }
            m_table->upsertDomains(ent_declared->declaration, table::entry_set_intersect(curr_domain, prev_domain));
        }
        if(mod_name && ent_all)
        {
            util::log("pql::eval", "Processing ModifiesP(EntName, AllEnt)");
            if(m_pkb->uses_modifies.getModifiesVars(mod_name->name).empty())
            {
                throw exception::PqlException("pql::eval",
                    "{} always evaluate to false. {} does not modify any variable.", modifies_p->toString(),
                    mod_name->name);
            }
            else
            {
                util::log("pql::eval", "{} always evaluate to true.", modifies_p->toString());
            }
        }
        if(mod_name && ent_name)
        {
            util::log("pql::eval", "Processing ModifiesP(EntName, EntName)");
            if(m_pkb->uses_modifies.getModifiesVars(mod_name->name).count(ent_name->name) > 0)
            {
                util::log("pql::eval", "{} always evaluate to true.", modifies_p->toString());
            }
            else
            {
                throw exception::PqlException("pql::eval", "{} always evaluate to false. {} does not modify {}.",
                    modifies_p->toString(), mod_name->name, ent_name->name);
            }
        }
    }
    void Evaluator::handleModifiesS(const ast::ModifiesS* modifies_s)
    {
        assert(modifies_s);
        assert(modifies_s->ent);
        assert(modifies_s->modifier);
        auto* mod_declared = dynamic_cast<ast::DeclaredStmt*>(modifies_s->modifier);
        auto* mod_stmt_id = dynamic_cast<ast::StmtId*>(modifies_s->modifier);
        auto* mod_all = dynamic_cast<ast::AllStmt*>(modifies_s->modifier);
        auto* ent_declared = dynamic_cast<ast::DeclaredEnt*>(modifies_s->ent);
        auto* ent_name = dynamic_cast<ast::EntName*>(modifies_s->ent);
        auto* ent_all = dynamic_cast<ast::AllEnt*>(modifies_s->ent);
        if(mod_all)
        {
            throw exception::PqlException(
                "pql::eval", "Modifier of ModifiesS cannot be '_': {}", modifies_s->toString());
        }
        if(mod_declared && (ast::kStmtDesignEntities.count(mod_declared->declaration->design_ent) == 0))
        {
            throw exception::PqlException(
                "pql::eval", "Declared modifier of ModifiesS must be a statement type: {}", modifies_s->toString());
        }
        if(ent_declared && (ent_declared->declaration->design_ent != ast::DESIGN_ENT::VARIABLE))
        {
            throw exception::PqlException(
                "pql::eval", "Entity being modified must be of type VARIABLE: {}", modifies_s->toString());
        }
        if(mod_declared && ent_declared)
        {
            util::log("pql::eval", "Processing ModifiesS(DeclaredStmt, DeclaredEnt)");
            // Entries for mod_declared are StmtNums
            for(const table::Entry& entry : m_table->getDomain(mod_declared->declaration))
            {
                std::unordered_set<std::string> ent_names =
                    this->m_pkb->uses_modifies.getModifiesVars(entry.getStmtNum());
                for(std::string ent_name : ent_names)
                {
                    table::Entry mod_entry = table::Entry(mod_declared->declaration, entry.getStmtNum());
                    table::Entry ent_entry = table::Entry(ent_declared->declaration, ent_name);
                    util::log("pql::eval", "{} adds Join({},{}),", modifies_s->toString(), mod_entry.toString(),
                        ent_entry.toString());
                    m_table->addJoin(mod_entry, ent_entry);
                }
            }
        }
        if(mod_declared && ent_name)
        {
            util::log("pql::eval", "Processing ModifiesS(DeclaredStmt, EntName)");

            std::unordered_set<std::string> modifier_candidates =
                m_pkb->uses_modifies.getModifies(mod_declared->declaration->design_ent, ent_name->name);
            if(modifier_candidates.empty())
            {
                throw pql::exception::PqlException("pql::eval",
                    "{} will always evaluate to false. No statements modifies {}", modifies_s->toString(),
                    ent_name->name);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(mod_declared->declaration);
                for(std::string stmt_num_str : modifier_candidates)
                {
                    simple::ast::StatementNum stmt_num = atoi(stmt_num_str.c_str());
                    auto entry = table::Entry(mod_declared->declaration, stmt_num);
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", modifies_s->toString(), entry.toString());
                }
                m_table->upsertDomains(mod_declared->declaration, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        if(mod_declared && ent_all)
        {
            util::log("pql::eval", "Processing ModifiesS(DeclaredStmt, _)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(mod_declared->declaration);
            for(const table::Entry& entry : m_table->getDomain(mod_declared->declaration))
            {
                simple::ast::StatementNum stmt_num = entry.getStmtNum();
                if(!m_pkb->uses_modifies.getModifiesVars(stmt_num).empty())
                {
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", modifies_s->toString(), entry.toString());
                }
            }
            m_table->upsertDomains(mod_declared->declaration, table::entry_set_intersect(prev_domain, curr_domain));
        }
        if(mod_stmt_id && ent_declared)
        {
            util::log("pql::eval", "Processing ModifiesS(StmtId, DeclaredEnt)");
            std::unordered_set<std::string> var_candidates = m_pkb->uses_modifies.getModifiesVars(mod_stmt_id->id);
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table->getDomain(ent_declared->declaration);
            for(const std::string& var : var_candidates)
            {
                util::log("pql::eval", "Adding StatementNum: {} modifies {}", mod_stmt_id->id, var);
                auto entry = table::Entry(ent_declared->declaration, var);
                curr_domain.insert(entry);
                util::log("pql::eval", "{} adds {} to curr domain", modifies_s->toString(), entry.toString());
            }
            m_table->upsertDomains(ent_declared->declaration, table::entry_set_intersect(curr_domain, prev_domain));
        }
        if(mod_stmt_id && ent_all)
        {
            util::log("pql::eval", "Processing ModifiesS(StmtId, AllEnt)");
            if(m_pkb->uses_modifies.getModifiesVars(mod_stmt_id->id).empty())
            {
                throw exception::PqlException("pql::eval",
                    "{} always evaluate to false. StatementNum {} does not modify any variable.",
                    modifies_s->toString(), mod_stmt_id->id);
            }
            else
            {
                util::log("pql::eval", "{} always evaluate to true.", modifies_s->toString());
            }
        }
        if(mod_stmt_id && ent_name)
        {
            util::log("pql::eval", "Processing ModifiesS(StmtId, EntName)");
            if(m_pkb->uses_modifies.getModifiesVars(mod_stmt_id->id).count(ent_name->name) > 0)
            {
                util::log("pql::eval", "{} always evaluate to true.", modifies_s->toString());
            }
            else
            {
                throw exception::PqlException("pql::eval",
                    "StatementNum: {} always evaluate to false. {} does not modify {}.", modifies_s->toString(),
                    mod_stmt_id->id, ent_name->name);
            }
        }
    }
    void Evaluator::handleParent(const ast::Parent* parent) { }
    void Evaluator::handleParentT(const ast::ParentT* parent_t) { }
}
