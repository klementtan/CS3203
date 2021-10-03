// pattern.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"


namespace pql::ast
{
    namespace s_ast = simple::ast;
    namespace table = pql::eval::table;

    using PqlException = util::PqlException;

    void AssignPatternCond::evaluate(const pkb::ProgramKB* pkb, table::Table* tbl) const
    {
        const auto& var_ent = this->ent;
        assert(this->assignment_declaration->design_ent == DESIGN_ENT::ASSIGN);

        if(var_ent.isDeclaration())
        {
            assert(var_ent.declaration()->design_ent == DESIGN_ENT::VARIABLE);
            tbl->addSelectDecl(var_ent.declaration());
        }

        tbl->addSelectDecl(assignment_declaration);

        // join pairs are needed in case of `pattern a(v, ...)` (two decls)
        std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

        auto domain = tbl->getDomain(this->assignment_declaration);
        for(auto it = domain.begin(); it != domain.end();)
        {
            bool should_erase = false;
            auto assign_stmt =
                dynamic_cast<const s_ast::AssignStmt*>(pkb->getStatementAt(it->getStmtNum()).getAstStmt());
            assert(assign_stmt);

            // check the rhs first, since it requires less table operations
            if(this->expr_spec.expr != nullptr)
            {
                if(this->expr_spec.is_subexpr)
                    should_erase |= !s_ast::partialMatch(this->expr_spec.expr.get(), assign_stmt->rhs.get());
                else
                    should_erase |= !s_ast::exactMatch(this->expr_spec.expr.get(), assign_stmt->rhs.get());
            }


            // don't do extra work if we're already going to yeet this
            if(!should_erase)
            {
                if(var_ent.isName())
                {
                    auto var_name = var_ent.name();
                    should_erase |= (assign_stmt->lhs != var_name);
                }
                else if(var_ent.isDeclaration())
                {
                    util::logfmt("pql::eval", "Processing pattern assign (v, ...)");
                    // in theory, we also need to check if there aren't any variables, and if so yeet this
                    // assignment from the domain; however, any valid SIMPLE program has at least one variable,
                    // so in reality this should not be triggered.
                    auto var_decl = var_ent.declaration();
                    auto var_list = tbl->getDomain(var_decl);
                    if(var_list.empty())
                        should_erase |= true;

                    for(const auto& entry : var_list)
                    {
                        auto var_name = entry.getVal();
                        if(var_name == assign_stmt->lhs)
                        {
                            // For 'pattern a (v,...)', when a = i, v must equal to the lhs
                            // of stmt i
                            allowed_entries.insert({ *it, entry });
                        }
                    }
                }
                else if(var_ent.isWildcard())
                {
                    // do nothing
                }
                else
                {
                    throw PqlException("pql::eval", "unreachable: invalid entity type");
                }
            }

            if(should_erase)
                it = domain.erase(it);
            else
                ++it;
        }
        if(var_ent.isDeclaration())
        {
            tbl->addJoin(table::Join(assignment_declaration, var_ent.declaration(), allowed_entries));
        }

        tbl->upsertDomains(this->assignment_declaration, domain);
    }

    void evaluate_if_while_pattern(
        const pkb::ProgramKB* pkb, table::Table* tbl, Declaration* stmt_decl, const EntRef& var_ent)
    {
        if(var_ent.isDeclaration())
        {
            assert(var_ent.declaration()->design_ent == DESIGN_ENT::VARIABLE);
            tbl->addSelectDecl(var_ent.declaration());
        }

        tbl->addSelectDecl(stmt_decl);

        // join pairs are needed in case of `pattern if/while (v, ...)` (two decls)
        std::unordered_set<std::pair<table::Entry, table::Entry>> join_pairs;

        auto domain = tbl->getDomain(stmt_decl);
        for(auto it = domain.begin(); it != domain.end();)
        {
            bool should_erase = false;
            auto& condition_vars = pkb->getStatementAt(it->getStmtNum()).getVariablesUsedInCondition();
            if(condition_vars.empty())
                should_erase |= true;

            if(!should_erase)
            {
                if(var_ent.isName())
                {
                    if(condition_vars.count(var_ent.name()) == 0)
                        should_erase |= true;
                }
                else if(var_ent.isDeclaration())
                {
                    util::logfmt("pql::eval", "Processing pattern while (v, ...)");

                    auto var_decl = var_ent.declaration();
                    auto var_list = tbl->getDomain(var_decl);
                    if(var_list.empty())
                        should_erase |= true;

                    for(const auto& entry : var_list)
                    {
                        auto var_name = entry.getVal();
                        if(condition_vars.count(var_name) > 0)
                            join_pairs.insert({ *it, entry });
                    }
                }
                else if(var_ent.isWildcard())
                {
                    // do nothing
                }
                else
                {
                    throw PqlException("pql::eval", "unreachable: invalid entity type");
                }
            }

            if(should_erase)
                it = domain.erase(it);
            else
                ++it;
        }

        if(var_ent.isDeclaration())
            tbl->addJoin(table::Join(stmt_decl, var_ent.declaration(), join_pairs));

        tbl->upsertDomains(stmt_decl, domain);
    }






    void IfPatternCond::evaluate(const pkb::ProgramKB* pkb, table::Table* tbl) const
    {
        const auto& var_ent = this->ent;
        assert(this->if_declaration->design_ent == DESIGN_ENT::IF);

        evaluate_if_while_pattern(pkb, tbl, this->if_declaration, var_ent);
    }

    void WhilePatternCond::evaluate(const pkb::ProgramKB* pkb, table::Table* tbl) const
    {
        const auto& var_ent = this->ent;
        assert(this->while_declaration->design_ent == DESIGN_ENT::WHILE);

        evaluate_if_while_pattern(pkb, tbl, this->while_declaration, var_ent);
    }
}
