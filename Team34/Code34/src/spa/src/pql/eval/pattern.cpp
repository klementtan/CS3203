// pattern.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"


namespace pql::eval
{
    void Evaluator::handlePattern(const ast::PatternCl& pattern)
    {
        for(auto p : pattern.pattern_conds)
            p->evaluate(this->m_pkb, this->m_table);
    }
}

namespace pql::ast
{
    namespace s_ast = simple::ast;
    namespace table = pql::eval::table;

    using PqlException = util::PqlException;

    void AssignPatternCond::evaluate(pkb::ProgramKB* pkb, table::Table* tbl) const
    {
        bool is_var_wild = dynamic_cast<AllEnt*>(this->ent);
        bool is_var_name = dynamic_cast<EntName*>(this->ent);
        bool is_var_decl = dynamic_cast<DeclaredEnt*>(this->ent);
        assert(this->assignment_declaration->design_ent == DESIGN_ENT::ASSIGN);

        if(is_var_decl)
            tbl->addSelectDecl(dynamic_cast<DeclaredEnt*>(this->ent)->declaration);
        tbl->addSelectDecl(assignment_declaration);
        // Stores dependency when pattern a(v, ...)
        std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

        auto domain = tbl->getDomain(this->assignment_declaration);
        for(auto it = domain.begin(); it != domain.end();)
        {
            bool should_erase = false;
            auto assign_stmt = dynamic_cast<s_ast::AssignStmt*>(pkb->getStatementAtIndex(it->getStmtNum())->stmt);
            assert(assign_stmt);

            // check the rhs first, since it requires less table operations
            assert(this->expr_spec);
            if(this->expr_spec->expr != nullptr)
            {
                if(this->expr_spec->is_subexpr)
                    should_erase |= !s_ast::partialMatch(this->expr_spec->expr, assign_stmt->rhs);
                else
                    should_erase |= !s_ast::exactMatch(this->expr_spec->expr, assign_stmt->rhs);
            }


            // don't do extra work if we're already going to yeet this
            if(!should_erase)
            {
                if(is_var_name)
                {
                    auto var_name = dynamic_cast<EntName*>(this->ent)->name;
                    should_erase |= (assign_stmt->lhs != var_name);
                }
                else if(is_var_decl)
                {
                    util::log("pql::eval", "Processing pattern a (v, ...)");
                    // in theory, we also need to check if there aren't any variables, and if so yeet this
                    // assignment from the domain; however, any valid SIMPLE program has at least one variable,
                    // so in reality this should not be triggered.
                    auto var_decl = dynamic_cast<DeclaredEnt*>(this->ent)->declaration;
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
                else if(is_var_wild)
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
        if(is_var_decl)
        {
            tbl->addJoin(table::Join(
                assignment_declaration, dynamic_cast<DeclaredEnt*>(this->ent)->declaration, allowed_entries));
        }

        tbl->upsertDomains(this->assignment_declaration, domain);
    }
}
