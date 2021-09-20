// pkb.cpp

#include <queue>
#include <cassert>
#include <algorithm>

#include <zpr.h>

#include "pkb.h"
#include "util.h"
#include "simple/ast.h"
#include "exceptions.h"

namespace pkb
{
    namespace s_ast = simple::ast;

    Statement* ProgramKB::getStatementAtIndex(s_ast::StatementNum stmt_no)
    {
        return const_cast<Statement*>(const_cast<const ProgramKB*>(this)->getStatementAtIndex(stmt_no));
    }

    const Statement* ProgramKB::getStatementAtIndex(s_ast::StatementNum stmt_no) const
    {
        if(stmt_no > m_statements.size() || stmt_no == 0)
            throw util::PkbException("pkb", "StatementNum is out of range");

        return &m_statements[stmt_no - 1];
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



    /**
     * Start of Parent methods
     */
    bool ProgramKB::isParent(s_ast::StatementNum fst, s_ast::StatementNum snd) const
    {
        if(auto it = m_direct_parents.find(snd); it != m_direct_parents.end())
            return it->second == fst;

        return false;
    }

    bool ProgramKB::isParentT(s_ast::StatementNum fst, s_ast::StatementNum snd) const
    {
        if(auto it = m_ancestors.find(snd); it != m_ancestors.end())
            return it->second.count(fst) > 0;

        return false;
    }

    std::optional<s_ast::StatementNum> ProgramKB::getParentOf(s_ast::StatementNum fst) const
    {
        // this will return 0 if it has no parent
        if(auto it = m_direct_parents.find(fst); it != m_direct_parents.end())
            return it->second;
        else
            return std::nullopt;
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getAncestorsOf(s_ast::StatementNum fst) const
    {
        if(auto it = m_ancestors.find(fst); it != m_ancestors.end())
            return it->second;
        else
            return {};
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getChildrenOf(s_ast::StatementNum fst) const
    {
        if(auto it = m_direct_children.find(fst); it != m_direct_children.end())
            return it->second;
        else
            return {};
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getDescendantsOf(s_ast::StatementNum fst) const
    {
        if(auto it = m_descendants.find(fst); it != m_descendants.end())
            return it->second;
        else
            return {};
    }

    bool ProgramKB::followsRelationExists() const
    {
        return this->m_follows_exists;
    }

    bool ProgramKB::parentRelationExists() const
    {
        return this->m_parent_exists;
    }
    /**
     * End of Parent methods
     */

    ProgramKB::ProgramKB(std::unique_ptr<simple::ast::Program> program)
    {
        this->m_program = std::move(program);
    }

    ProgramKB::~ProgramKB()
    {
        // for(auto follow : this->follows)
        //     delete follow;
    }

    const simple::ast::Program* ProgramKB::getProgram() const
    {
        return this->m_program.get();
    }
}
