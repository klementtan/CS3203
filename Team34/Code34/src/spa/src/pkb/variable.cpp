// variable.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

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

    std::unordered_set<StmtNum> Variable::getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT ent) const
    {
        if(ent == pql::ast::DESIGN_ENT::PROCEDURE)
            throw util::PkbException("pkb", "invalid design entity for getUsingStmtNumsFiltered");

        std::unordered_set<StmtNum> ret {};
        for(const auto* stmt : m_used_by)
            if(design_entity_matches(stmt->getAstStmt(), ent))
                ret.insert(stmt->getAstStmt()->id);

        return ret;
    }

    std::unordered_set<StmtNum> Variable::getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT ent) const
    {
        if(ent == pql::ast::DESIGN_ENT::PROCEDURE)
            throw util::PkbException("pkb", "invalid design entity for getModifyingStmtNumsFiltered");

        std::unordered_set<StmtNum> ret {};
        for(const auto* stmt : m_modified_by)
            if(design_entity_matches(stmt->getAstStmt(), ent))
                ret.insert(stmt->getAstStmt()->id);

        return ret;
    }

    std::unordered_set<std::string> Variable::getUsingProcNames() const
    {
        std::unordered_set<std::string> ret {};
        for(const auto* proc : m_used_by_procs)
            ret.insert(proc->getAstProc()->name);

        return ret;
    }

    std::unordered_set<std::string> Variable::getModifyingProcNames() const
    {
        std::unordered_set<std::string> ret {};
        for(const auto* proc : m_modified_by_procs)
            ret.insert(proc->getAstProc()->name);

        return ret;
    }

}
