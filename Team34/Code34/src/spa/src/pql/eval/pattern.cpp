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
        for(const auto& p : pattern.pattern_conds)
            p->evaluate(this->m_pkb, &this->m_table);
    }
}

namespace pql::ast
{
    namespace s_ast = simple::ast;
    namespace table = pql::eval::table;

    using PqlException = util::PqlException;

    void AssignPatternCond::evaluate(pkb::ProgramKB* pkb, table::Table* tbl) const
    {
        const auto& var_ent = this->ent;
        assert(this->assignment_declaration->design_ent == DESIGN_ENT::ASSIGN);

        if(var_ent.isDeclaration())
            tbl->addSelectDecl(var_ent.declaration());

        tbl->addSelectDecl(assignment_declaration);

        // Stores dependency when pattern a(v, ...)
        std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries;

        auto domain = tbl->getDomain(this->assignment_declaration);
        for(auto it = domain.begin(); it != domain.end();)
        {
            bool should_erase = false;
            auto assign_stmt =
                dynamic_cast<const s_ast::AssignStmt*>(pkb->getStatementAtIndex(it->getStmtNum())->getAstStmt());
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
                    util::log("pql::eval", "Processing pattern a (v, ...)");
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
}
