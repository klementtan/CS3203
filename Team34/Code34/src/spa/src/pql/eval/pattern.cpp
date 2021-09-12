// pattern.cpp

#include <cassert>
#include <algorithm>

#include "pql/exception.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

using PqlException = pql::exception::PqlException;

namespace pql::eval
{
    void Evaluator::handlePattern(const ast::PatternCl* pattern)
    {
        assert(pattern);
        for(auto p : pattern->pattern_conds)
            p->evaluate(this->m_pkb, this->m_table);
    }
}

namespace pql::ast
{
    namespace s_ast = simple::ast;
    namespace table = pql::eval::table;

    void AssignPatternCond::evaluate(pkb::ProgramKB* pkb, table::Table* tbl) const
    {
        bool is_var_wild = dynamic_cast<AllEnt*>(this->ent);
        bool is_var_name = dynamic_cast<EntName*>(this->ent);
        bool is_var_decl = dynamic_cast<DeclaredEnt*>(this->ent);

        assert(this->assignment_declaration->design_ent == DESIGN_ENT::ASSIGN);

        auto domain = tbl->getDomain(this->assignment_declaration);
        for(auto it = domain.begin(); it != domain.end(); )
        {
            bool should_erase = false;
            auto assign_stmt = dynamic_cast<s_ast::AssignStmt*>(pkb->uses_modifies.statements[it->getStmtNum() - 1]->stmt);
            assert(assign_stmt);

            // check the rhs first, since it requires less table operations
            assert(this->expr_spec);
            if(this->expr_spec->expr != nullptr)
            {
                if(this->expr_spec->is_subexpr)
                    ;
                else
                    ;
            }




            if(is_var_name)
            {
                auto var_name = dynamic_cast<EntName*>(this->ent)->name;
                should_erase |= (assign_stmt->lhs != var_name);
            }
            else if(!should_erase && is_var_decl)
            {

            }
            else if(is_var_wild)
            {
                // do nothing
            }
            else
            {
                throw PqlException("pql::eval", "unreachable");
            }

            if(should_erase)
                it = domain.erase(it);
            else
                ++it;
        }
    }
}
