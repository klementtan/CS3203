// variable.cpp

#include "pkb.h"
#include "exceptions.h"

namespace pkb
{
    namespace s_ast = simple::ast;
    using StmtNum = s_ast::StatementNum;

    static bool design_entity_matches(const s_ast::Stmt* stmt, pql::ast::DESIGN_ENT ent)
    {
        using pql::ast::DESIGN_ENT;

        switch(ent)
        {
            case DESIGN_ENT::STMT:
            case DESIGN_ENT::PROG_LINE:
                return true;
            case DESIGN_ENT::READ:
                return dynamic_cast<const s_ast::ReadStmt*>(stmt);
            case DESIGN_ENT::PRINT:
                return dynamic_cast<const s_ast::PrintStmt*>(stmt);
            case DESIGN_ENT::CALL:
                return dynamic_cast<const s_ast::ProcCall*>(stmt);
            case DESIGN_ENT::WHILE:
                return dynamic_cast<const s_ast::WhileLoop*>(stmt);
            case DESIGN_ENT::IF:
                return dynamic_cast<const s_ast::IfStmt*>(stmt);
            case DESIGN_ENT::ASSIGN:
                return dynamic_cast<const s_ast::AssignStmt*>(stmt);

            case DESIGN_ENT::VARIABLE:
            case DESIGN_ENT::CONSTANT:
            case DESIGN_ENT::PROCEDURE:
                return false;

            default:
                throw util::PkbException("pkb", "invalid design entity");
        }
    }

    StatementSet Variable::getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT ent) const
    {
        if(ent == pql::ast::DESIGN_ENT::PROCEDURE)
            throw util::PkbException("pkb", "invalid design entity for getUsingStmtNumsFiltered");

        StatementSet ret {};
        for(const auto* stmt : m_used_by)
            if(design_entity_matches(stmt->getAstStmt(), ent))
                ret.insert(stmt->getStmtNum());

        return ret;
    }

    StatementSet Variable::getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT ent) const
    {
        if(ent == pql::ast::DESIGN_ENT::PROCEDURE)
            throw util::PkbException("pkb", "invalid design entity for getModifyingStmtNumsFiltered");

        StatementSet ret {};
        for(const auto* stmt : m_modified_by)
            if(design_entity_matches(stmt->getAstStmt(), ent))
                ret.insert(stmt->getStmtNum());

        return ret;
    }

    const std::unordered_set<std::string>& Variable::getUsingProcNames() const
    {
        return m_used_by_procs;
    }

    const std::unordered_set<std::string>& Variable::getModifyingProcNames() const
    {
        return m_modified_by_procs;
    }

}
