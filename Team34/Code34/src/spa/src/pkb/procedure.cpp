// procedure.cpp

#include "pkb.h"

namespace pkb
{
    Procedure::Procedure(const simple::ast::Procedure* ast_proc) : m_ast_proc(ast_proc) { }

    bool Procedure::usesVariable(const std::string& varname) const
    {
        return m_uses.count(varname) > 0;
    }

    bool Procedure::modifiesVariable(const std::string& varname) const
    {
        return m_modifies.count(varname) > 0;
    }

    const simple::ast::Procedure* Procedure::getAstProc() const
    {
        return m_ast_proc;
    }

    const std::unordered_set<std::string>& Procedure::getUsedVariables() const
    {
        return m_uses;
    }

    const std::unordered_set<std::string>& Procedure::getModifiedVariables() const
    {
        return m_modifies;
    }

    std::string Procedure::getName() const
    {
        return m_ast_proc->name;
    }
}
