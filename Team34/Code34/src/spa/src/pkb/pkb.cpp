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
        if(stmt_no > m_statements.size())
        {
            throw util::PkbException("pkb", "Statement is out of range");
        }
        return &m_statements[stmt_no - 1];
    }

    const Procedure& ProgramKB::getProcedureNamed(const std::string& name) const
    {
        return m_procedures.at(name);
    }

    Procedure& ProgramKB::addProcedure(const std::string& name, simple::ast::Procedure* proc)
    {
        auto& p = m_procedures[name];
        if(p.ast_proc != nullptr)
            throw util::PkbException("pkb", "duplicate definition of procedure '{}'", name);

        p.ast_proc = proc;
        return p;
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

    /**
     * Start of Uses and Modifies methods
     */
    bool ProgramKB::isUses(const simple::ast::StatementNum& stmt_num, const std::string& var) const
    {
        if(m_variables.find(var) == m_variables.end() || stmt_num > m_statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        const auto& stmt = m_statements.at(stmt_num - 1);
        if(auto c = dynamic_cast<const s_ast::ProcCall*>(stmt.getAstStmt()))
        {
            return m_procedures.at(c->proc_name).uses.count(var) > 0;
        }
        else
        {
            return stmt.usesVariable(var);
        }
    }

    bool ProgramKB::isUses(const std::string& proc, const std::string& var) const
    {
        if(m_variables.find(var) == m_variables.end() || m_procedures.find(proc) == m_procedures.end())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        return m_procedures.at(proc).uses.count(var) > 0;
    }

    std::unordered_set<std::string> ProgramKB::getUsesVars(const std::string& var) const
    {
        if(m_procedures.find(var) == m_procedures.end())
        {
            throw util::PkbException("pkb::eval", "Procedure not found.");
        }
        return m_procedures.at(var).uses;
    }

    std::unordered_set<std::string> ProgramKB::getUses(const pql::ast::DESIGN_ENT& type, const std::string& var) const
    {
        if(m_variables.find(var) == m_variables.end())
        {
            throw util::PkbException("pkb::eval", "Variable not found.");
        }
        std::unordered_set<std::string> uses;
        auto& stmt_list = m_variables.at(var).used_by;
        switch(type)
        {
            case pql::ast::DESIGN_ENT::ASSIGN:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::AssignStmt*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PRINT:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::PrintStmt*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::STMT:
                for(auto& stmt : stmt_list)
                    uses.insert(std::to_string(stmt->id));

                break;
            case pql::ast::DESIGN_ENT::CALL:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::ProcCall*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::IF:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::IfStmt*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::WHILE:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::WhileLoop*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PROCEDURE:
                for(auto& proc : m_variables.at(var).used_by_procs)
                    uses.insert(proc->name);
                break;
            default:
                throw util::PkbException("pkb::eval", "Invalid statement type.");
        }
        return uses;
    }

    bool ProgramKB::isModifies(const simple::ast::StatementNum& stmt_num, const std::string& var) const
    {
        if(m_variables.find(var) == m_variables.end() || stmt_num > m_statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        auto& stmt = m_statements.at(stmt_num - 1);
        if(auto c = dynamic_cast<const s_ast::ProcCall*>(stmt.getAstStmt()))
        {
            return m_procedures.at(c->proc_name).modifies.count(var) > 0;
        }
        else
        {
            return stmt.modifiesVariable(var);
        }
    }

    bool ProgramKB::isModifies(const std::string& proc, const std::string& var) const
    {
        if(m_variables.find(var) == m_variables.end() || m_procedures.find(proc) == m_procedures.end())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        return m_procedures.at(proc).modifies.count(var) > 0;
    }

    std::unordered_set<std::string> ProgramKB::getModifiesVars(const std::string& proc) const
    {
        if(m_procedures.find(proc) == m_procedures.end())
        {
            throw util::PkbException("pkb::eval", "Procedure {} not found.", proc);
        }
        return m_procedures.at(proc).modifies;
    }

    std::unordered_set<std::string> ProgramKB::getModifies(const pql::ast::DESIGN_ENT& type, const std::string& var) const
    {
        if(m_variables.find(var) == m_variables.end())
        {
            throw util::PkbException("pkb::eval", "Variable not found.");
        }
        std::unordered_set<std::string> modifies;
        auto& stmt_list = m_variables.at(var).modified_by;
        switch(type)
        {
            case pql::ast::DESIGN_ENT::ASSIGN:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::AssignStmt*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::READ:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::ReadStmt*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::STMT:
                for(auto& stmt : stmt_list)
                {
                    modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::CALL:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::ProcCall*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::IF:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::IfStmt*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::WHILE:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<const s_ast::WhileLoop*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PROCEDURE:
                for(auto& proc : m_variables.at(var).modified_by_procs)
                    modifies.insert(proc->name);

                break;
            default:
                throw util::PkbException("pkb::eval", "Invalid statement type.");
        }
        return modifies;
    }
    /**
     * End of Uses and Modifies methods
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
