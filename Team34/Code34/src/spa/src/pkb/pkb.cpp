// pkb.cpp

#include <queue>
#include <algorithm>

#include <zpr.h>

#include "pkb.h"
#include "util.h"
#include "simple/ast.h"
#include "exceptions.h"

namespace pkb
{
    Statement& ProgramKB::getStatementAt(const StatementNum& stmt_no)
    {
        return const_cast<Statement&>(const_cast<const ProgramKB*>(this)->getStatementAt(stmt_no));
    }

    const Statement& ProgramKB::getStatementAt(const StatementNum& stmt_no) const
    {
        if(stmt_no > m_statements.size() || stmt_no == 0)
            throw util::PkbException("pkb", "StatementNum is out of range");

        return m_statements[stmt_no - 1];
    }

    const Procedure& ProgramKB::getProcedureNamed(const std::string& name) const
    {
        if(auto it = m_procedures.find(name); it != m_procedures.end())
            return it->second;

        throw util::PkbException("pkb", "no procedure named '{}'", name);
    }

    Procedure& ProgramKB::getProcedureNamed(const std::string& name)
    {
        return const_cast<Procedure&>(const_cast<const ProgramKB*>(this)->getProcedureNamed(name));
    }

    Procedure& ProgramKB::addProcedure(const std::string& name, const simple::ast::Procedure* proc)
    {
        if(auto it = m_procedures.find(name); it != m_procedures.end())
            throw util::PkbException("pkb", "duplicate definition of procedure '{}'", name);

        return m_procedures.emplace(name, proc).first->second;
    }

    const Variable& ProgramKB::getVariableNamed(const std::string& name) const
    {
        if(auto it = m_variables.find(name); it != m_variables.end())
            return it->second;

        throw util::PkbException("pkb", "no variable named '{}'", name);
    }

    Variable& ProgramKB::getVariableNamed(const std::string& name)
    {
        return const_cast<Variable&>(const_cast<const ProgramKB*>(this)->getVariableNamed(name));
    }

    const Variable* ProgramKB::maybeGetVariableNamed(const std::string& name) const
    {
        if(auto it = m_variables.find(name); it != m_variables.end())
            return &it->second;

        return nullptr;
    }

    const Procedure* ProgramKB::maybeGetProcedureNamed(const std::string& name) const
    {
        if(auto it = m_procedures.find(name); it != m_procedures.end())
            return &it->second;

        return nullptr;
    }


    void ProgramKB::addConstant(std::string value)
    {
        m_constants.insert(std::move(value));
    }

    const std::vector<Statement>& ProgramKB::getAllStatements() const
    {
        return m_statements;
    }

    const std::unordered_set<std::string>& ProgramKB::getAllConstants() const
    {
        return m_constants;
    }

    const std::unordered_map<std::string, Procedure>& ProgramKB::getAllProcedures() const
    {
        return m_procedures;
    }

    const std::unordered_map<std::string, Variable>& ProgramKB::getAllVariables() const
    {
        return m_variables;
    }

    const StatementSet& ProgramKB::getAllStatementsOfKind(pql::ast::DESIGN_ENT ent) const
    {
        return m_stmt_kinds.at(ent);
    }

    bool ProgramKB::followsRelationExists() const
    {
        return this->m_follows_exists;
    }

    bool ProgramKB::parentRelationExists() const
    {
        return this->m_parent_exists;
    }

    bool ProgramKB::callsRelationExists() const
    {
        return this->m_calls_exists;
    }

    bool ProgramKB::nextRelationExists() const
    {
        return m_cfg->nextRelationExists();
    }

    bool ProgramKB::affectsRelationExists() const
    {
        return m_cfg->affectsRelationExists();
    }

    bool ProgramKB::nextBipRelationExists() const
    {
        return m_cfg->nextBipRelationExists();
    }

    bool ProgramKB::affectsBipRelationExists() const
    {
        return m_cfg->affectsBipRelationExists();
    }


    const pkb::CFG* ProgramKB::getCFG() const
    {
        return m_cfg.get();
    }

    ProgramKB::ProgramKB(std::unique_ptr<simple::ast::Program> program)
    {
        this->m_program = std::move(program);
    }

    ProgramKB::~ProgramKB() { }

    const simple::ast::Program* ProgramKB::getProgram() const
    {
        return this->m_program.get();
    }
}
