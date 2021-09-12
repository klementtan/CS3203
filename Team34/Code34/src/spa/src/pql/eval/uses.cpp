// uses.cpp

#include <cassert>
#include <algorithm>

#include "pql/exception.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = pql::exception::PqlException;

    void Evaluator::handleUsesP(const ast::UsesP* rel)
    {
        assert(rel);
        bool is_proc_name = dynamic_cast<ast::EntName*>(rel->user);
        bool is_proc_decl = dynamic_cast<ast::DeclaredEnt*>(rel->user);
        bool is_proc_all = dynamic_cast<ast::AllEnt*>(rel->user);
        bool is_var_name = dynamic_cast<ast::EntName*>(rel->ent);
        bool is_var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent);
        bool is_var_all = dynamic_cast<ast::AllEnt*>(rel->ent);

        // this should not happen, since Uses(_, foo) is invalid according to the specs
        if(is_proc_all)
            throw PqlException("pql::eval", "first argument of Uses cannot be '_'");

        if(is_proc_decl &&
            dynamic_cast<ast::DeclaredEnt*>(rel->user)->declaration->design_ent != ast::DESIGN_ENT::PROCEDURE)
            throw PqlException("pql::eval", "entity for first argument of Uses must be a procedure");

        if(is_var_decl &&
            dynamic_cast<ast::DeclaredEnt*>(rel->ent)->declaration->design_ent != ast::DESIGN_ENT::VARIABLE)
            throw PqlException("pql::eval", "entity for second argument of Uses must be a variable");


        if(is_proc_name && is_var_name)
        {
            auto proc_name = dynamic_cast<ast::EntName*>(rel->user);
            auto var_name = dynamic_cast<ast::EntName*>(rel->ent);

            util::log("pql::eval", "Processing UsesP(EntName, EntName)");
            if(!m_pkb->uses_modifies.isUses(proc_name->name, var_name->name))
                throw PqlException("pql::eval", "{} is always false", rel->toString(), var_name->name);
        }
        else if(is_proc_name && is_var_decl)
        {
            auto proc_name = dynamic_cast<ast::EntName*>(rel->user);
            auto var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent);


            util::log("pql::eval", "Processing UsesP(EntName, DeclaredStmt)");
            auto used_vars = m_pkb->uses_modifies.getUsesVars(proc_name->name);
            if(used_vars.empty())
                throw PqlException("pql::eval", "{} is always false; {} doesn't use any variables", rel->toString());

            std::unordered_set<table::Entry> new_domain {};

            for(const auto& var : used_vars)
                new_domain.insert(table::Entry(var_decl->declaration, var));

            auto old_domain = m_table->getDomain(var_decl->declaration);
            m_table->upsertDomains(var_decl->declaration, table::entry_set_intersect(old_domain, new_domain));
        }
        else if(is_proc_name && is_var_all)
        {
            auto proc_name = dynamic_cast<ast::EntName*>(rel->user);

            util::log("pql::eval", "Processing UsesP(EntName, _)");
            auto used_vars = m_pkb->uses_modifies.getUsesVars(proc_name->name);
            if(used_vars.empty())
                throw PqlException("pql::eval", "{} is always false; {} doesn't use any variables", rel->toString());
        }

        else if(is_proc_decl && is_var_name)
        {
            auto proc_decl = dynamic_cast<ast::DeclaredEnt*>(rel->user);
            auto var_name = dynamic_cast<ast::EntName*>(rel->ent);

            util::log("pql::eval", "Processing UsesP(DeclaredEnt, EntName)");

            auto procs_using = m_pkb->uses_modifies.getUses(ast::DESIGN_ENT::PROCEDURE, var_name->name);
            if(procs_using.empty())
                throw PqlException(
                    "pql::eval", "{} is always false; {} no procedure uses '{}'", rel->toString(), var_name->name);

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& proc_name : procs_using)
                new_domain.insert(table::Entry(proc_decl->declaration, proc_name));

            auto old_domain = m_table->getDomain(proc_decl->declaration);
            m_table->upsertDomains(proc_decl->declaration, table::entry_set_intersect(old_domain, new_domain));
        }
        else if(is_proc_decl && is_var_decl)
        {
            auto proc_decl = dynamic_cast<ast::DeclaredEnt*>(rel->user);
            auto var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent);

            util::log("pql::eval", "Processing UsesP(DeclaredEnt, DeclaredStmt)");
            for(const auto& entry : m_table->getDomain(proc_decl->declaration))
            {
                auto proc_used_vars = m_pkb->uses_modifies.getUsesVars(entry.getVal());
                if(proc_used_vars.empty())
                    continue;

                auto proc_entry = table::Entry(proc_decl->declaration, entry.getVal());
                for(const auto& var_name : proc_used_vars)
                {
                    auto var_entry = table::Entry(var_decl->declaration, var_name);
                    util::log("pql::eval", "{} adds Join({}, {}),", rel->toString(), proc_entry.toString(),
                        var_entry.toString());

                    m_table->addJoin(proc_entry, var_entry);
                }
            }
        }
        else if(is_proc_decl && is_var_all)
        {
            auto proc_decl = dynamic_cast<ast::DeclaredEnt*>(rel->user);

            util::log("pql::eval", "Processing UsesP(DeclaredEnt, _)");
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table->getDomain(proc_decl->declaration))
            {
                auto proc_used_vars = m_pkb->uses_modifies.getUsesVars(entry.getVal());
                if(proc_used_vars.empty())
                    continue;

                new_domain.insert(entry);
            }

            m_table->upsertDomains(proc_decl->declaration,
                table::entry_set_intersect(new_domain, m_table->getDomain(proc_decl->declaration)));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }

    void Evaluator::handleUsesS(const ast::UsesS* rel)
    {
        assert(rel);
        bool is_user_sid = dynamic_cast<ast::StmtId*>(rel->user);
        bool is_user_decl = dynamic_cast<ast::DeclaredStmt*>(rel->user);
        bool is_var_name = dynamic_cast<ast::EntName*>(rel->ent);
        bool is_var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent);
        bool is_var_all = dynamic_cast<ast::AllEnt*>(rel->ent);

        // this should not happen, since Uses(_, foo) is invalid according to the specs
        if(dynamic_cast<ast::AllEnt*>(rel->user))
            throw PqlException("pql::eval", "first argument of Uses cannot be '_'");

        if(dynamic_cast<ast::EntName*>(rel->user))
            throw PqlException("pql::eval", "UsesS should not have an entity name as its first argument");

        if(is_var_decl &&
            dynamic_cast<ast::DeclaredEnt*>(rel->ent)->declaration->design_ent != ast::DESIGN_ENT::VARIABLE)
            throw PqlException("pql::eval", "entity for second argument of Uses must be a variable");

        if(is_user_decl &&
            (ast::kStmtDesignEntities.count(dynamic_cast<ast::DeclaredStmt*>(rel->user)->declaration->design_ent) == 0))
            throw PqlException("pql::eval", "first argument for UsesS must be a statement entity");

        if(is_user_sid && is_var_name)
        {
            auto user_sid = dynamic_cast<ast::StmtId*>(rel->user);
            auto var_name = dynamic_cast<ast::EntName*>(rel->ent);

            util::log("pql::eval", "Processing UsesS(StmtId, EntName)");
            if(!m_pkb->uses_modifies.isUses(user_sid->id, var_name->name))
                throw PqlException("pql::eval", "{} is always false", rel->toString(), var_name->name);
        }
        else if(is_user_sid && is_var_decl)
        {
            auto user_sid = dynamic_cast<ast::StmtId*>(rel->user);
            auto var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent);

            util::log("pql::eval", "Processing UsesS(StmtId, DeclaredEnt)");
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& var : m_pkb->uses_modifies.getUsesVars(user_sid->id))
                new_domain.insert(table::Entry(var_decl->declaration, var));

            m_table->upsertDomains(var_decl->declaration,
                table::entry_set_intersect(new_domain, m_table->getDomain(var_decl->declaration)));
        }
        else if(is_user_sid && is_var_all)
        {
            auto user_sid = dynamic_cast<ast::StmtId*>(rel->user);

            util::log("pql::eval", "Processing UsesS(StmtId, _)");
            if(m_pkb->uses_modifies.getUsesVars(user_sid->id).empty())
                throw PqlException("pql::eval", "{} is always false", rel->toString());
        }
        else if(is_user_decl && is_var_name)
        {
            auto user_decl = dynamic_cast<ast::DeclaredStmt*>(rel->user);
            auto var_name = dynamic_cast<ast::EntName*>(rel->ent);

            util::log("pql::eval", "Processing UsesS(DeclaredStmt, EntName)");
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& var : m_pkb->uses_modifies.getUses(user_decl->declaration->design_ent, var_name->name))
                new_domain.insert(
                    table::Entry(user_decl->declaration, static_cast<simple::ast::StatementNum>(std::stoi(var))));

            m_table->upsertDomains(user_decl->declaration,
                table::entry_set_intersect(new_domain, m_table->getDomain(user_decl->declaration)));
        }
        else if(is_user_decl && is_var_decl)
        {
            auto user_decl = dynamic_cast<ast::DeclaredStmt*>(rel->user);
            auto var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent);

            util::log("pql::eval", "Processing UsesS(DeclaredStmt, DeclaredEnt)");

            for(const auto& entry : m_table->getDomain(user_decl->declaration))
            {
                auto used_vars = m_pkb->uses_modifies.getUsesVars(entry.getStmtNum());
                auto user_entry = table::Entry(user_decl->declaration, entry.getStmtNum());

                for(const auto& var_name : used_vars)
                {
                    auto var_entry = table::Entry(var_decl->declaration, var_name);
                    m_table->addJoin(user_entry, var_entry);
                }
            }
        }
        else if(is_user_decl && is_var_all)
        {
            auto user_decl = dynamic_cast<ast::DeclaredStmt*>(rel->user);

            util::log("pql::eval", "Processing UsesS(DeclaredStmt, _)");

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& entry : m_table->getDomain(user_decl->declaration))
            {
                auto used_vars = m_pkb->uses_modifies.getUsesVars(entry.getVal());
                if(used_vars.empty())
                    continue;

                new_domain.insert(entry);
            }

            m_table->upsertDomains(user_decl->declaration,
                table::entry_set_intersect(new_domain, m_table->getDomain(user_decl->declaration)));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }
}
