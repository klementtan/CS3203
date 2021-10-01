// uses_modifies.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using namespace pkb;
    using PqlException = util::PqlException;

    // specifically for uses and modifies. they are more similar to each other than they are to the other relations.
    struct UsesModifiesRelationAbstractor
    {
        const char* relationName = nullptr;

        std::function<std::unordered_set<std::string>(const Statement&)> getStmtRelatedVariables {};

        // proc.getUsedVariables/getModifiedVariables
        std::function<const std::unordered_set<std::string>&(const Procedure&)> getProcRelatedVariables {};

        // var.getUsingProcs/getModifyingProcs
        std::function<const std::unordered_set<std::string>&(const Variable&)> getVariableRelatedProcs {};


        std::function<bool(const Procedure&, const std::string&)> procedureRelatesVariable {};
        std::function<bool(const Statement&, const std::string&)> statementRelatesVariable {};

        void evaluate(const ProgramKB* pkb, table::Table* table, const ast::RelCond* rel,
            const ast::StmtRef& stmt, const ast::EntRef& right) const;

        void evaluate(const ProgramKB* pkb, table::Table* table, const ast::RelCond* rel,
            const ast::EntRef& proc, const ast::EntRef& right) const;
    };


    void Evaluator::handleUsesP(const ast::UsesP* rel)
    {
        static UsesModifiesRelationAbstractor abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "UsesP";

            abs.getProcRelatedVariables = [](const Procedure& p) -> decltype(auto) {
                return p.getUsedVariables();
            };

            abs.getVariableRelatedProcs = [](const Variable& v) -> decltype(auto) {
                return v.getUsingProcNames();
            };

            abs.procedureRelatesVariable = [](const Procedure& p, const std::string& s) -> bool {
                return p.usesVariable(s);
            };
        }

        abs.evaluate(m_pkb, &m_table, rel, rel->user, rel->ent);
    }

    void Evaluator::handleModifiesP(const ast::ModifiesP* rel)
    {
        static UsesModifiesRelationAbstractor abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "ModifiesP";

            abs.getProcRelatedVariables = [](const Procedure& p) -> decltype(auto) {
                return p.getModifiedVariables();
            };

            abs.getVariableRelatedProcs = [](const Variable& v) -> decltype(auto) {
                return v.getModifyingProcNames();
            };

            abs.procedureRelatesVariable = [](const Procedure& p, const std::string& s) -> bool {
                return p.modifiesVariable(s);
            };
        }

        abs.evaluate(m_pkb, &m_table, rel, rel->modifier, rel->ent);
    }











    // Uses/ModifiesP
    void UsesModifiesRelationAbstractor::evaluate(const ProgramKB* pkb, table::Table* table,
        const ast::RelCond* rel, const ast::EntRef& proc_ent, const ast::EntRef& var_ent) const
    {
        if(proc_ent.isDeclaration())
            table->addSelectDecl(proc_ent.declaration());

        if(var_ent.isDeclaration())
            table->addSelectDecl(var_ent.declaration());

        // this should not happen, since Uses(_, foo) is invalid according to the specs
        if(proc_ent.isWildcard())
            throw PqlException("pql::eval", "first argument of Uses/Modifies cannot be '_'");

        if(proc_ent.isDeclaration() && proc_ent.declaration()->design_ent != ast::DESIGN_ENT::PROCEDURE)
            throw PqlException("pql::eval", "entity for first argument of {} must be a procedure", this->relationName);

        if(var_ent.isDeclaration() && var_ent.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE)
            throw PqlException("pql::eval", "entity for second argument of {} must be a variable", this->relationName);

        if(proc_ent.isName() && var_ent.isName())
        {
            auto proc_name = proc_ent.name();
            auto var_name = var_ent.name();

            util::logfmt("pql::eval", "Processing {}(EntName, EntName)", this->relationName);
            if(!this->procedureRelatesVariable(pkb->getProcedureNamed(proc_name), var_name))
                throw PqlException("pql::eval", "{} is always false", rel->toString(), var_name);
        }
        else if(proc_ent.isName() && var_ent.isDeclaration())
        {
            auto proc_name = proc_ent.name();
            auto var_decl = var_ent.declaration();

            util::logfmt("pql::eval", "Processing {}(EntName, DeclaredStmt)", this->relationName);

            auto& used_vars = this->getProcRelatedVariables(pkb->getProcedureNamed(proc_name));
            if(used_vars.empty())
                throw PqlException("pql::eval", "{} is always false; {} doesn't use any variables", rel->toString());

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& var : used_vars)
                new_domain.emplace(var_decl, var);

            auto old_domain = table->getDomain(var_decl);
            table->upsertDomains(var_decl, table::entry_set_intersect(old_domain, new_domain));
        }
        else if(proc_ent.isName() && var_ent.isWildcard())
        {
            auto proc_name = proc_ent.name();

            util::logfmt("pql::eval", "Processing {}(EntName, _)", this->relationName);
            auto& used_vars = this->getProcRelatedVariables(pkb->getProcedureNamed(proc_name));
            if(used_vars.empty())
                throw PqlException("pql::eval", "{} is always false; {} doesn't use any variables", rel->toString());
        }

        else if(proc_ent.isDeclaration() && var_ent.isName())
        {
            auto proc_decl = proc_ent.declaration();
            auto var_name = var_ent.name();

            util::logfmt("pql::eval", "Processing {}(DeclaredEnt, EntName)", this->relationName);

            const auto& procs_using = this->getVariableRelatedProcs(pkb->getVariableNamed(var_name));
            if(procs_using.empty())
                throw PqlException(
                    "pql::eval", "{} is always false; {} no procedure uses '{}'", rel->toString(), var_name);

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& proc_name : procs_using)
                new_domain.emplace(proc_decl, proc_name);

            auto old_domain = table->getDomain(proc_decl);
            table->upsertDomains(proc_decl, table::entry_set_intersect(old_domain, new_domain));
        }
        else if(proc_ent.isDeclaration() && var_ent.isDeclaration())
        {
            auto proc_decl = proc_ent.declaration();
            auto var_decl = var_ent.declaration();

            util::logfmt("pql::eval", "Processing {}(DeclaredEnt, DeclaredStmt)", this->relationName);

            auto proc_domain = table->getDomain(proc_decl);
            auto new_var_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

            for(auto it = proc_domain.begin(); it != proc_domain.end();)
            {
                auto& used_vars = this->getProcRelatedVariables(pkb->getProcedureNamed(it->getVal()));
                if(used_vars.empty())
                {
                    it = proc_domain.erase(it);
                    continue;
                }

                auto proc_entry = table::Entry(proc_decl, it->getVal());
                for(const auto& var_name : used_vars)
                {
                    auto var_entry = table::Entry(var_decl, var_name);
                    util::logfmt("pql::eval", "{} adds Join({}, {}),", rel->toString(), proc_entry.toString(),
                        var_entry.toString());
                    allowed_entries.insert({ proc_entry, var_entry });
                    new_var_domain.insert(var_entry);
                }
                ++it;
            }

            table->upsertDomains(proc_decl, proc_domain);
            table->upsertDomains(var_decl, table::entry_set_intersect(new_var_domain, table->getDomain(var_decl)));
            table->addJoin(table::Join(proc_decl, var_decl, allowed_entries));
        }
        else if(proc_ent.isDeclaration() && var_ent.isWildcard())
        {
            auto proc_decl = proc_ent.declaration();

            util::logfmt("pql::eval", "Processing {}(DeclaredEnt, _)", this->relationName);
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& entry : table->getDomain(proc_decl))
            {
                auto& proc_used_vars = this->getProcRelatedVariables(pkb->getProcedureNamed(entry.getVal()));
                if(proc_used_vars.empty())
                    continue;

                new_domain.insert(entry);
            }

            table->upsertDomains(proc_decl, table::entry_set_intersect(new_domain, table->getDomain(proc_decl)));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }




    // Uses/ModifiesS
    void UsesModifiesRelationAbstractor::evaluate(const ProgramKB* pkb, table::Table* table,
        const ast::RelCond* rel, const ast::StmtRef& stmt, const ast::EntRef& right) const
    {

    }
}
