// modifies.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = util::PqlException;

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
            throw PqlException("pql::eval", "Modifier of ModifiesP cannot be '_': {}", modifies_p->toString());
        }
        if(mod_declared && (mod_declared->declaration->design_ent != ast::DESIGN_ENT::PROCEDURE))
        {
            throw PqlException("pql::eval", "Declared modifier of ModifiesP can only be of type PROCEDURE: {}",
                modifies_p->toString());
        }
        if(ent_declared && (ent_declared->declaration->design_ent != ast::DESIGN_ENT::VARIABLE))
        {
            throw PqlException(
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
                throw PqlException("pql::eval", "{} will always evaluate to false. No procedures modifies {}",
                    modifies_p->toString(), ent_name->name);
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
                throw PqlException("pql::eval", "{} always evaluate to false. {} does not modify any variable.",
                    modifies_p->toString(), mod_name->name);
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
                throw PqlException("pql::eval", "{} always evaluate to false. {} does not modify {}.",
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
            throw PqlException("pql::eval", "Modifier of ModifiesS cannot be '_': {}", modifies_s->toString());
        }
        if(mod_declared && (ast::kStmtDesignEntities.count(mod_declared->declaration->design_ent) == 0))
        {
            throw PqlException(
                "pql::eval", "Declared modifier of ModifiesS must be a statement type: {}", modifies_s->toString());
        }
        if(ent_declared && (ent_declared->declaration->design_ent != ast::DESIGN_ENT::VARIABLE))
        {
            throw PqlException(
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
                for(const auto& ent_name : ent_names)
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
                throw PqlException("pql::eval", "{} will always evaluate to false. No statements modifies {}",
                    modifies_s->toString(), ent_name->name);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table->getDomain(mod_declared->declaration);
                for(const auto& stmt_num_str : modifier_candidates)
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
                throw PqlException("pql::eval",
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
                throw PqlException("pql::eval", "StatementNum: {} always evaluate to false. {} does not modify {}.",
                    modifies_s->toString(), mod_stmt_id->id, ent_name->name);
            }
        }
    }
}
