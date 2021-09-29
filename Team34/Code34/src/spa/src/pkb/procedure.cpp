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

    bool Procedure::callsProcedure(const std::string& procname) const
    {
        return m_calls.count(procname) > 0;
    }

    bool Procedure::callsProcedureTransitively(const std::string& procname) const
    {
        return m_calls_transitive.count(procname) > 0;
    }

    bool Procedure::isCalledByProcedure(const std::string& procname) const
    {
        return m_called_by.count(procname) > 0;
    }

    bool Procedure::isTransitivelyCalledByProcedure(const std::string& procname) const
    {
        return m_called_by_transitive.count(procname) > 0;
    }

    const std::unordered_set<std::string>& Procedure::getAllCallers() const
    {
        return m_called_by;
    }

    const std::unordered_set<std::string>& Procedure::getAllCalledProcedures() const
    {
        return m_calls;
    }

    const std::unordered_set<std::string>& Procedure::getAllTransitiveCallers() const
    {
        return m_called_by_transitive;
    }

    const std::unordered_set<std::string>& Procedure::getAllTransitivelyCalledProcedures() const
    {
        return m_calls_transitive;
    }
}
