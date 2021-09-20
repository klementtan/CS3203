// modifies.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = util::PqlException;

    void Evaluator::handleModifiesP(const ast::ModifiesP* rel)
    {
        assert(rel);

        const auto& ent_ent = rel->ent;
        const auto& modifier_ent = rel->modifier;

        if(modifier_ent.isDeclaration())
            m_table.addSelectDecl(modifier_ent.declaration());

        if(ent_ent.isDeclaration())
            m_table.addSelectDecl(ent_ent.declaration());

        if(modifier_ent.isWildcard())
        {
            throw PqlException("pql::eval", "Modifier of ModifiesP cannot be '_': {}", rel->toString());
        }
        else if(modifier_ent.isDeclaration() && (modifier_ent.declaration()->design_ent != ast::DESIGN_ENT::PROCEDURE))
        {
            throw PqlException(
                "pql::eval", "Declared modifier of ModifiesP can only be of type PROCEDURE: {}", rel->toString());
        }
        else if(ent_ent.isDeclaration() && (ent_ent.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE))
        {
            throw PqlException("pql::eval", "Entity being modified must be of type VARIABLE: {}", rel->toString());
        }
        else if(modifier_ent.isDeclaration() && ent_ent.isDeclaration())
        {
            auto mod_decl = modifier_ent.declaration();
            auto ent_decl = ent_ent.declaration();

            util::log("pql::eval", "Processing ModifiesP(DeclaredEnt, DeclaredEnt)");

            auto mod_domain = m_table.getDomain(mod_decl);
            auto new_ent_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = mod_domain.begin(); it != mod_domain.end();)
            {
                auto modified_vars = m_pkb->getModifiesVars(it->getVal());
                if(modified_vars.empty())
                {
                    it = mod_domain.erase(it);
                    continue;
                }

                auto mod_entry = table::Entry(mod_decl, it->getVal());
                for(const auto& var_name : modified_vars)
                {
                    auto ent_entry = table::Entry(ent_decl, var_name);
                    util::log("pql::eval", "{} adds Join({}, {})", rel->toString(), mod_entry.toString(),
                        ent_entry.toString());

                    allowed_entries.insert({ mod_entry, ent_entry });
                    new_ent_domain.insert(ent_entry);
                }
                ++it;
            }

            m_table.upsertDomains(mod_decl, mod_domain);
            m_table.upsertDomains(ent_decl, table::entry_set_intersect(new_ent_domain, m_table.getDomain(ent_decl)));
            m_table.addJoin(table::Join(mod_decl, ent_decl, allowed_entries));
        }
        else if(modifier_ent.isDeclaration() && ent_ent.isName())
        {
            auto mod_decl = modifier_ent.declaration();
            auto ent_name = ent_ent.name();

            util::log("pql::eval", "Processing ModifiesP(DeclaredEnt, EntName)");

            std::unordered_set<std::string> modifier_candidates = m_pkb->getModifies(mod_decl->design_ent, ent_name);
            if(modifier_candidates.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false. No procedures modifies {}",
                    rel->toString(), ent_name);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table.getDomain(mod_decl);
                for(std::string proc_name : modifier_candidates)
                {
                    auto entry = table::Entry(mod_decl, proc_name);
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", rel->toString(), entry.toString());
                }
                m_table.upsertDomains(mod_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(modifier_ent.isDeclaration() && ent_ent.isWildcard())
        {
            auto mod_decl = modifier_ent.declaration();

            util::log("pql::eval", "Processing ModifiesP(DeclaredEnt, _)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table.getDomain(mod_decl);
            for(const table::Entry& entry : m_table.getDomain(mod_decl))
            {
                std::string proc_name = entry.getVal();
                if(!m_pkb->getModifiesVars(proc_name).empty())
                {
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", rel->toString(), entry.toString());
                }
            }
            m_table.upsertDomains(mod_decl, table::entry_set_intersect(prev_domain, curr_domain));
        }
        else if(modifier_ent.isName() && ent_ent.isDeclaration())
        {
            auto mod_name = modifier_ent.name();
            auto ent_decl = ent_ent.declaration();

            util::log("pql::eval", "Processing ModifiesP(EntName, DeclaredEnt)");
            std::unordered_set<std::string> var_candidates = m_pkb->getModifiesVars(mod_name);
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table.getDomain(ent_decl);
            for(const std::string& var : var_candidates)
            {
                util::log("pql::eval", "Adding {} modifies {}", mod_name, var);
                auto entry = table::Entry(ent_decl, var);
                curr_domain.insert(entry);
                util::log("pql::eval", "{} adds {} to curr domain", rel->toString(), entry.toString());
            }
            m_table.upsertDomains(ent_decl, table::entry_set_intersect(curr_domain, prev_domain));
        }
        else if(modifier_ent.isName() && ent_ent.isWildcard())
        {
            auto mod_name = modifier_ent.name();

            util::log("pql::eval", "Processing ModifiesP(EntName, AllEnt)");
            if(m_pkb->getModifiesVars(mod_name).empty())
            {
                throw PqlException("pql::eval", "{} always evaluate to false. {} does not modify any variable.",
                    rel->toString(), mod_name);
            }
            else
            {
                util::log("pql::eval", "{} always evaluate to true.", rel->toString());
            }
        }
        else if(modifier_ent.isName() && ent_ent.isName())
        {
            auto mod_name = modifier_ent.name();
            auto ent_name = ent_ent.name();

            util::log("pql::eval", "Processing ModifiesP(EntName, EntName)");
            if(m_pkb->getModifiesVars(mod_name).count(ent_name) > 0)
            {
                util::log("pql::eval", "{} always evaluate to true.", rel->toString());
            }
            else
            {
                throw PqlException("pql::eval", "{} always evaluate to false. {} does not modify {}.", rel->toString(),
                    mod_name, ent_name);
            }
        }
        else
        {
            throw PqlException("pql::eval", "unreachable: invalid combination of argument types");
        }
    }



    void Evaluator::handleModifiesS(const ast::ModifiesS* rel)
    {
        assert(rel);

        const auto& modifier_stmt = rel->modifier;
        const auto& ent_ent = rel->ent;

        if(modifier_stmt.isDeclaration())
            m_table.addSelectDecl(modifier_stmt.declaration());

        if(ent_ent.isDeclaration())
            m_table.addSelectDecl(ent_ent.declaration());

        if(modifier_stmt.isWildcard())
        {
            throw PqlException("pql::eval", "Modifier of ModifiesS cannot be '_': {}", rel->toString());
        }
        else if(modifier_stmt.isDeclaration() &&
                (ast::kStmtDesignEntities.count(modifier_stmt.declaration()->design_ent) == 0))
        {
            throw PqlException(
                "pql::eval", "Declared modifier of ModifiesS must be a statement type: {}", rel->toString());
        }
        else if(ent_ent.isDeclaration() && (ent_ent.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE))
        {
            throw PqlException("pql::eval", "Entity being modified must be of type VARIABLE: {}", rel->toString());
        }
        else if(modifier_stmt.isDeclaration() && ent_ent.isDeclaration())
        {
            auto mod_decl = modifier_stmt.declaration();
            auto ent_decl = ent_ent.declaration();

            util::log("pql::eval", "Processing ModifiesS(DeclaredStmt, DeclaredEnt)");

            auto mod_domain = m_table.getDomain(mod_decl);
            auto new_ent_domain = table::Domain {};

            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;
            for(auto it = mod_domain.begin(); it != mod_domain.end();)
            {
                auto& modified_vars = m_pkb->getStatementAtIndex(it->getStmtNum())->getModifiedVariables();
                if(modified_vars.empty())
                {
                    it = mod_domain.erase(it);
                    continue;
                }

                auto mod_entry = table::Entry(mod_decl, it->getStmtNum());
                for(const auto& var_name : modified_vars)
                {
                    auto ent_entry = table::Entry(ent_decl, var_name);
                    util::log("pql::eval", "{} adds Join({}, {}),", rel->toString(), mod_entry.toString(),
                        ent_entry.toString());
                    allowed_entries.insert({ mod_entry, ent_entry });

                    new_ent_domain.insert(ent_entry);
                }
                ++it;
            }

            m_table.upsertDomains(mod_decl, mod_domain);
            m_table.upsertDomains(ent_decl, table::entry_set_intersect(new_ent_domain, m_table.getDomain(ent_decl)));
            m_table.addJoin(table::Join(mod_decl, ent_decl, allowed_entries));
        }
        else if(modifier_stmt.isDeclaration() && ent_ent.isName())
        {
            auto mod_decl = modifier_stmt.declaration();
            auto ent_name = ent_ent.name();

            util::log("pql::eval", "Processing ModifiesS(DeclaredStmt, EntName)");

            std::unordered_set<std::string> modifier_candidates = m_pkb->getModifies(mod_decl->design_ent, ent_name);
            if(modifier_candidates.empty())
            {
                throw PqlException("pql::eval", "{} will always evaluate to false. No statements modifies {}",
                    rel->toString(), ent_name);
            }
            else
            {
                std::unordered_set<table::Entry> curr_domain;
                std::unordered_set<table::Entry> prev_domain = m_table.getDomain(mod_decl);
                for(const auto& stmt_num_str : modifier_candidates)
                {
                    simple::ast::StatementNum stmt_num = atoi(stmt_num_str.c_str());
                    auto entry = table::Entry(mod_decl, stmt_num);
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", rel->toString(), entry.toString());
                }
                m_table.upsertDomains(mod_decl, table::entry_set_intersect(prev_domain, curr_domain));
            }
        }
        else if(modifier_stmt.isDeclaration() && ent_ent.isWildcard())
        {
            auto mod_decl = modifier_stmt.declaration();

            util::log("pql::eval", "Processing ModifiesS(DeclaredStmt, _)");
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table.getDomain(mod_decl);
            for(const table::Entry& entry : m_table.getDomain(mod_decl))
            {
                auto stmt = m_pkb->getStatementAtIndex(entry.getStmtNum());
                if(!stmt->getModifiedVariables().empty())
                {
                    curr_domain.insert(entry);
                    util::log("pql::eval", "{} adds {} to curr domain", rel->toString(), entry.toString());
                }
            }
            m_table.upsertDomains(mod_decl, table::entry_set_intersect(prev_domain, curr_domain));
        }
        else if(modifier_stmt.isStatementId() && ent_ent.isDeclaration())
        {
            auto mod_stmt_id = modifier_stmt.id();
            auto ent_decl = ent_ent.declaration();

            util::log("pql::eval", "Processing ModifiesS(StmtId, DeclaredEnt)");
            auto& var_candidates = m_pkb->getStatementAtIndex(mod_stmt_id)->getModifiedVariables();
            std::unordered_set<table::Entry> curr_domain;
            std::unordered_set<table::Entry> prev_domain = m_table.getDomain(ent_decl);
            for(const std::string& var : var_candidates)
            {
                util::log("pql::eval", "Adding StatementNum: {} modifies {}", mod_stmt_id, var);
                auto entry = table::Entry(ent_decl, var);
                curr_domain.insert(entry);
                util::log("pql::eval", "{} adds {} to curr domain", rel->toString(), entry.toString());
            }
            m_table.upsertDomains(ent_decl, table::entry_set_intersect(curr_domain, prev_domain));
        }
        else if(modifier_stmt.isStatementId() && ent_ent.isWildcard())
        {
            auto mod_stmt_id = modifier_stmt.id();

            util::log("pql::eval", "Processing ModifiesS(StmtId, AllEnt)");
            if(m_pkb->getStatementAtIndex(mod_stmt_id)->getModifiedVariables().empty())
            {
                throw PqlException("pql::eval",
                    "{} always evaluate to false. StatementNum {} does not modify any variable.", rel->toString(),
                    mod_stmt_id);
            }
            else
            {
                util::log("pql::eval", "{} always evaluate to true.", rel->toString());
            }
        }
        else if(modifier_stmt.isStatementId() && ent_ent.isName())
        {
            auto mod_stmt_id = modifier_stmt.id();
            auto ent_name = ent_ent.name();

            util::log("pql::eval", "Processing ModifiesS(StmtId, EntName)");
            if(m_pkb->getStatementAtIndex(mod_stmt_id)->modifiesVariable(ent_name))
            {
                util::log("pql::eval", "{} always evaluate to true.", rel->toString());
            }
            else
            {
                throw PqlException("pql::eval", "StatementNum: {} always evaluate to false. {} does not modify {}.",
                    rel->toString(), mod_stmt_id, ent_name);
            }
        }
        else
        {
            throw PqlException("pql::eval", "unreachable: invalid combination of argument types");
        }
    }
}
