// uses_modifies.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/common.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using namespace pkb;
    using PqlException = util::PqlException;

    // specifically for uses and modifies. they are more similar to each other than they are to the other relations.
    struct UsesModifiesRelationAbstractor
    {
        const char* relationName = nullptr;

        std::function<const std::unordered_set<std::string>&(const Statement&)> getStmtRelatedVariables {};

        // proc.getUsedVariables/getModifiedVariables
        std::function<const std::unordered_set<std::string>&(const Procedure&)> getProcRelatedVariables {};

        // var.getUsingProcs/getModifyingProcs
        std::function<const std::unordered_set<std::string>&(const Variable&)> getVariableRelatedProcs {};

        std::function<StatementSet(const Variable&, ast::DESIGN_ENT)> getVariableRelatedStmts {};


        std::function<bool(const Procedure&, const std::string&)> procedureRelatesVariable {};
        std::function<bool(const Statement&, const std::string&)> statementRelatesVariable {};

        void evaluateS(const ProgramKB* pkb, table::Table* table, const ast::RelCond* rel, const ast::StmtRef& stmt,
            const ast::EntRef& right) const;

        void evaluateP(const ProgramKB* pkb, table::Table* table, const ast::RelCond* rel, const ast::EntRef& proc,
            const ast::EntRef& right) const;
    };
}

namespace pql::ast
{
    using namespace pkb;
    namespace table = pql::eval::table;

    void UsesP::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        static eval::UsesModifiesRelationAbstractor abs {};
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

        abs.evaluateP(pkb, tbl, this, this->user, this->ent);
    }

    void UsesS::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        static eval::UsesModifiesRelationAbstractor abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "UsesS";

            abs.getStmtRelatedVariables = [](const Statement& s) -> decltype(auto) {
                return s.getUsedVariables();
            };

            abs.getVariableRelatedStmts = [](const Variable& v, ast::DESIGN_ENT ent) -> decltype(auto) {
                return v.getUsingStmtNumsFiltered(ent);
            };

            abs.statementRelatesVariable = [](const Statement& s, const std::string& v) -> bool {
                return s.usesVariable(v);
            };
        }

        abs.evaluateS(pkb, tbl, this, this->user, this->ent);
    }

    void ModifiesP::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        static eval::UsesModifiesRelationAbstractor abs {};
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

        abs.evaluateP(pkb, tbl, this, this->modifier, this->ent);
    }

    void ModifiesS::evaluate(const ProgramKB* pkb, table::Table* tbl) const
    {
        static eval::UsesModifiesRelationAbstractor abs {};
        if(abs.relationName == nullptr)
        {
            abs.relationName = "ModifiesS";

            abs.getStmtRelatedVariables = [](const Statement& s) -> decltype(auto) {
                return s.getModifiedVariables();
            };

            abs.getVariableRelatedStmts = [](const Variable& v, ast::DESIGN_ENT ent) -> decltype(auto) {
                return v.getModifyingStmtNumsFiltered(ent);
            };

            abs.statementRelatesVariable = [](const Statement& s, const std::string& v) -> bool {
                return s.modifiesVariable(v);
            };
        }

        abs.evaluateS(pkb, tbl, this, this->modifier, this->ent);
    }
}








namespace pql::eval
{
    // Uses/ModifiesP
    void UsesModifiesRelationAbstractor::evaluateP(const ProgramKB* pkb, table::Table* table, const ast::RelCond* rel,
        const ast::EntRef& proc_ent, const ast::EntRef& var_ent) const
    {
        assert(rel);

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
            table->putDomain(var_decl, table::entry_set_intersect(old_domain, new_domain));
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
            table->putDomain(proc_decl, table::entry_set_intersect(old_domain, new_domain));
        }
        else if(proc_ent.isDeclaration() && var_ent.isDeclaration())
        {
            auto proc_decl = proc_ent.declaration();
            auto var_decl = var_ent.declaration();

            util::logfmt("pql::eval", "Processing {}(DeclaredEnt, DeclaredStmt)", this->relationName);

            evaluateTwoDeclRelations<std::string, std::string>(
                pkb, table, rel, proc_decl, var_decl, [&](const std::string& p) -> decltype(auto) {
                    return this->getProcRelatedVariables(pkb->getProcedureNamed(p));
                });
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

            table->putDomain(proc_decl, table::entry_set_intersect(new_domain, table->getDomain(proc_decl)));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }




    // Uses/ModifiesS
    void UsesModifiesRelationAbstractor::evaluateS(const ProgramKB* pkb, table::Table* table, const ast::RelCond* rel,
        const ast::StmtRef& user_stmt, const ast::EntRef& var_ent) const
    {
        assert(rel);

        if(user_stmt.isDeclaration())
            table->addSelectDecl(user_stmt.declaration());

        if(var_ent.isDeclaration())
            table->addSelectDecl(var_ent.declaration());

        // this should not happen, since Uses(_, foo) is invalid according to the specs
        if(user_stmt.isWildcard())
            throw PqlException("pql::eval", "first argument of Uses/Modifies cannot be '_'", this->relationName);

        if(var_ent.isDeclaration() && var_ent.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE)
            throw PqlException("pql::eval", "entity for second argument of {} must be a variable", this->relationName);

        if(user_stmt.isDeclaration() && (ast::getStmtDesignEntities().count(user_stmt.declaration()->design_ent) == 0))
            throw PqlException("pql::eval", "first argument for {} must be a statement entity", this->relationName);

        if(user_stmt.isStatementId() && var_ent.isName())
        {
            auto user_sid = user_stmt.id();
            auto var_name = var_ent.name();

            util::logfmt("pql::eval", "Processing {}(StmtId, EntName)", this->relationName);
            if(!this->statementRelatesVariable(pkb->getStatementAt(user_sid), var_name))
                throw PqlException("pql::eval", "{} is always false", rel->toString(), var_name);
        }
        else if(user_stmt.isStatementId() && var_ent.isDeclaration())
        {
            auto user_sid = user_stmt.id();
            auto var_decl = var_ent.declaration();

            util::logfmt("pql::eval", "Processing {}(StmtId, DeclaredEnt)", this->relationName);
            std::unordered_set<table::Entry> new_domain {};

            for(const auto& var : this->getStmtRelatedVariables(pkb->getStatementAt(user_sid)))
                new_domain.emplace(var_decl, var);

            table->putDomain(var_decl, table::entry_set_intersect(new_domain, table->getDomain(var_decl)));
        }
        else if(user_stmt.isStatementId() && var_ent.isWildcard())
        {
            auto user_sid = user_stmt.id();

            util::logfmt("pql::eval", "Processing {}(StmtId, _)", this->relationName);
            if(this->getStmtRelatedVariables(pkb->getStatementAt(user_sid)).empty())
                throw PqlException("pql::eval", "{} is always false", rel->toString());
        }
        else if(user_stmt.isDeclaration() && var_ent.isName())
        {
            auto user_decl = user_stmt.declaration();
            auto var_name = var_ent.name();
            auto& var = pkb->getVariableNamed(var_name);

            util::logfmt("pql::eval", "Processing {}(DeclaredStmt, EntName)", this->relationName);
            std::unordered_set<table::Entry> new_domain {};

            assert(user_decl->design_ent != ast::DESIGN_ENT::PROCEDURE);

            for(auto sid : this->getVariableRelatedStmts(var, user_decl->design_ent))
                new_domain.emplace(user_decl, sid);

            table->putDomain(user_decl, table::entry_set_intersect(new_domain, table->getDomain(user_decl)));
        }
        else if(user_stmt.isDeclaration() && var_ent.isDeclaration())
        {
            auto user_decl = user_stmt.declaration();
            auto var_decl = var_ent.declaration();

            util::logfmt("pql::eval", "Processing {}(DeclaredStmt, DeclaredEnt)", this->relationName);

            evaluateTwoDeclRelations<StatementNum, std::string>(
                pkb, table, rel, user_decl, var_decl, [&](const StatementNum& s) -> decltype(auto) {
                    return this->getStmtRelatedVariables(pkb->getStatementAt(s));
                });
        }
        else if(user_stmt.isDeclaration() && var_ent.isWildcard())
        {
            auto user_decl = user_stmt.declaration();

            util::logfmt("pql::eval", "Processing {}(DeclaredStmt, _)", this->relationName);

            std::unordered_set<table::Entry> new_domain {};
            for(const auto& entry : table->getDomain(user_decl))
            {
                decltype(auto) used_vars = this->getStmtRelatedVariables(pkb->getStatementAt(entry.getStmtNum()));
                if(used_vars.empty())
                    continue;

                new_domain.insert(entry);
            }

            table->putDomain(user_decl, table::entry_set_intersect(new_domain, table->getDomain(user_decl)));
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }
}
