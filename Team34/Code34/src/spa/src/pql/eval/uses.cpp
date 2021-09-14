// uses.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = util::PqlException;

    void Evaluator::handleUsesP(const ast::UsesP* rel)
    {
        assert(rel);
        bool is_proc_name = dynamic_cast<ast::EntName*>(rel->user);
        bool is_proc_decl = dynamic_cast<ast::DeclaredEnt*>(rel->user);
        bool is_var_name = dynamic_cast<ast::EntName*>(rel->ent);
        bool is_var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent);
        bool is_var_all = dynamic_cast<ast::AllEnt*>(rel->ent);

        // this should not happen, since Uses(_, foo) is invalid according to the specs
        if(dynamic_cast<ast::AllEnt*>(rel->user) || dynamic_cast<ast::AllStmt*>(rel->user))
            throw PqlException("pql::eval", "first argument of Uses cannot be '_'");

        if(is_proc_decl &&
            dynamic_cast<ast::DeclaredEnt*>(rel->user)->declaration->design_ent != ast::DESIGN_ENT::PROCEDURE)
            throw PqlException("pql::eval", "entity for first argument of Uses must be a procedure");

        if(is_var_decl &&
            dynamic_cast<ast::DeclaredEnt*>(rel->ent)->declaration->design_ent != ast::DESIGN_ENT::VARIABLE)
            throw PqlException("pql::eval", "entity for second argument of Uses must be a variable");


        if(is_proc_name && is_var_name)
        {
            auto proc_name = dynamic_cast<ast::EntName*>(rel->user)->name;
            auto var_name = dynamic_cast<ast::EntName*>(rel->ent)->name;

            util::log("pql::eval", "Processing UsesP(EntName, EntName)");
            if(!m_pkb->uses_modifies.isUses(proc_name, var_name))
                throw PqlException("pql::eval", "{} is always false", rel->toString(), var_name);
        }
        else if(is_proc_name && is_var_decl)
        {
            auto proc_name = dynamic_cast<ast::EntName*>(rel->user)->name;
            auto var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent)->declaration;


            util::log("pql::eval", "Processing UsesP(EntName, DeclaredStmt)");
            auto used_vars = m_pkb->uses_modifies.getUsesVars(proc_name);
            if(used_vars.empty())
                throw PqlException("pql::eval", "{} is always false; {} doesn't use any variables", rel->toString());

            std::unordered_set<table::Entry> new_domain {};

            for(const auto& var : used_vars)
                new_domain.emplace(var_decl, var);

            auto old_domain = m_table->getDomain(var_decl);
            m_table->upsertDomains(var_decl, table::entry_set_intersect(old_domain, new_domain));
        }
        else if(is_proc_name && is_var_all)
        {
            auto proc_name = dynamic_cast<ast::EntName*>(rel->user)->name;

            util::log("pql::eval", "Processing UsesP(EntName, _)");
            auto used_vars = m_pkb->uses_modifies.getUsesVars(proc_name);
            if(used_vars.empty())
                throw PqlException("pql::eval", "{} is always false; {} doesn't use any variables", rel->toString());
        }

        else if(is_proc_decl && is_var_name)
        {
            auto proc_decl = dynamic_cast<ast::DeclaredEnt*>(rel->user)->declaration;
            auto var_name = dynamic_cast<ast::EntName*>(rel->ent)->name;

            util::log("pql::eval", "Processing UsesP(DeclaredEnt, EntName)");

            auto procs_using = m_pkb->uses_modifies.getUses(ast::DESIGN_ENT::PROCEDURE, var_name);
            if(procs_using.empty())
                throw PqlException(
                    "pql::eval", "{} is always false; {} no procedure uses '{}'", rel->toString(), var_name);

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& proc_name : procs_using)
                new_domain.emplace(proc_decl, proc_name);

            auto old_domain = m_table->getDomain(proc_decl);
            m_table->upsertDomains(proc_decl, table::entry_set_intersect(old_domain, new_domain));
        }
        else if(is_proc_decl && is_var_decl)
        {
            auto proc_decl = dynamic_cast<ast::DeclaredEnt*>(rel->user)->declaration;
            auto var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent)->declaration;

            util::log("pql::eval", "Processing UsesP(DeclaredEnt, DeclaredStmt)");

            auto proc_domain = m_table->getDomain(proc_decl);
            auto new_var_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = proc_domain.begin(); it != proc_domain.end();)
            {
                auto used_vars = m_pkb->uses_modifies.getUsesVars(it->getVal());
                if(used_vars.empty())
                {
                    it = proc_domain.erase(it);
                    continue;
                }

                auto proc_entry = table::Entry(proc_decl, it->getVal());
                for(const auto& var_name : used_vars)
                {
                    auto var_entry = table::Entry(var_decl, var_name);
                    util::log("pql::eval", "{} adds Join({}, {}),", rel->toString(), proc_entry.toString(),
                        var_entry.toString());
                    allowed_entries.insert({ proc_entry, var_entry });
                    new_var_domain.insert(var_entry);
                }
                ++it;
            }

            m_table->upsertDomains(proc_decl, proc_domain);
            m_table->upsertDomains(var_decl, table::entry_set_intersect(new_var_domain, m_table->getDomain(var_decl)));
            m_table->addJoin(table::Join(proc_decl, var_decl, allowed_entries));
        }
        else if(is_proc_decl && is_var_all)
        {
            auto proc_decl = dynamic_cast<ast::DeclaredEnt*>(rel->user)->declaration;

            util::log("pql::eval", "Processing UsesP(DeclaredEnt, _)");
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : m_table->getDomain(proc_decl))
            {
                auto proc_used_vars = m_pkb->uses_modifies.getUsesVars(entry.getVal());
                if(proc_used_vars.empty())
                    continue;

                new_domain.insert(entry);
            }

            m_table->upsertDomains(proc_decl, table::entry_set_intersect(new_domain, m_table->getDomain(proc_decl)));
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
        if(dynamic_cast<ast::AllEnt*>(rel->user) || dynamic_cast<ast::AllStmt*>(rel->user))
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
            auto user_sid = dynamic_cast<ast::StmtId*>(rel->user)->id;
            auto var_name = dynamic_cast<ast::EntName*>(rel->ent)->name;

            util::log("pql::eval", "Processing UsesS(StmtId, EntName)");
            if(!m_pkb->uses_modifies.isUses(user_sid, var_name))
                throw PqlException("pql::eval", "{} is always false", rel->toString(), var_name);
        }
        else if(is_user_sid && is_var_decl)
        {
            auto user_sid = dynamic_cast<ast::StmtId*>(rel->user)->id;
            auto var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent)->declaration;

            util::log("pql::eval", "Processing UsesS(StmtId, DeclaredEnt)");
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& var : m_pkb->uses_modifies.getUsesVars(user_sid))
                new_domain.emplace(var_decl, var);

            m_table->upsertDomains(var_decl, table::entry_set_intersect(new_domain, m_table->getDomain(var_decl)));
        }
        else if(is_user_sid && is_var_all)
        {
            auto user_sid = dynamic_cast<ast::StmtId*>(rel->user)->id;

            util::log("pql::eval", "Processing UsesS(StmtId, _)");
            if(m_pkb->uses_modifies.getUsesVars(user_sid).empty())
                throw PqlException("pql::eval", "{} is always false", rel->toString());
        }
        else if(is_user_decl && is_var_name)
        {
            auto user_decl = dynamic_cast<ast::DeclaredStmt*>(rel->user)->declaration;
            auto var_name = dynamic_cast<ast::EntName*>(rel->ent)->name;

            util::log("pql::eval", "Processing UsesS(DeclaredStmt, EntName)");
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& stmt : m_pkb->uses_modifies.getUses(user_decl->design_ent, var_name))
                new_domain.emplace(user_decl, static_cast<simple::ast::StatementNum>(std::stoi(stmt)));

            m_table->upsertDomains(user_decl, table::entry_set_intersect(new_domain, m_table->getDomain(user_decl)));
        }
        else if(is_user_decl && is_var_decl)
        {
            auto user_decl = dynamic_cast<ast::DeclaredStmt*>(rel->user)->declaration;
            auto var_decl = dynamic_cast<ast::DeclaredEnt*>(rel->ent)->declaration;

            util::log("pql::eval", "Processing UsesS(DeclaredStmt, DeclaredEnt)");

            auto user_domain = m_table->getDomain(user_decl);
            auto new_var_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = user_domain.begin(); it != user_domain.end();)
            {
                auto used_vars = m_pkb->uses_modifies.getUsesVars(it->getStmtNum());
                if(used_vars.empty())
                {
                    it = user_domain.erase(it);
                    continue;
                }

                auto user_entry = table::Entry(user_decl, it->getStmtNum());
                for(const auto& var_name : used_vars)
                {
                    auto var_entry = table::Entry(var_decl, var_name);
                    util::log("pql::eval", "{} adds Join({}, {}),", rel->toString(), user_entry.toString(),
                        var_entry.toString());
                    allowed_entries.insert({ user_entry, var_entry });
                    new_var_domain.insert(var_entry);
                }
                ++it;
            }

            m_table->upsertDomains(user_decl, user_domain);
            m_table->upsertDomains(var_decl, table::entry_set_intersect(new_var_domain, m_table->getDomain(var_decl)));
            m_table->addJoin(table::Join(user_decl, var_decl, allowed_entries));
        }
        else if(is_user_decl && is_var_all)
        {
            auto user_decl = dynamic_cast<ast::DeclaredStmt*>(rel->user)->declaration;

            util::log("pql::eval", "Processing UsesS(DeclaredStmt, _)");

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& entry : m_table->getDomain(user_decl))
            {
                auto used_vars = m_pkb->uses_modifies.getUsesVars(entry.getStmtNum());
                if(used_vars.empty())
                    continue;

                new_domain.insert(entry);
            }

            m_table->upsertDomains(user_decl, table::entry_set_intersect(new_domain, m_table->getDomain(user_decl)));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }
}