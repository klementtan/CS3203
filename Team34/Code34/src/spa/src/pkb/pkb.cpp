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
        if(stmt_no > m_statements.size())
        {
            throw util::PkbException("pkb", "Statement is out of range");
        }
        // return this->uses_modifies.statements.at(stmt_no - 1);
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
        this->_constants.insert(std::move(value));
    }

    // Takes in two 1-indexed StatementNums
    // bool ProgramKB::isFollows(s_ast::StatementNum fst, s_ast::StatementNum snd)
    // {
    //     // thinking of more elegant ways of handling this hmm
    //     if(fst > this->follows.size() || snd > this->follows.size() || fst < 1 || snd < 1)
    //         throw util::PkbException("pkb::eval", "StatementNum out of range.");

    //     return this->follows[fst - 1]->directly_after == snd;
    // }

    // // Same as isFollows
    // bool ProgramKB::isFollowsT(s_ast::StatementNum fst, s_ast::StatementNum snd)
    // {
    //     if(fst > this->follows.size() || snd > this->follows.size() || fst < 1 || snd < 1)
    //         throw util::PkbException("pkb::eval", "StatementNum out of range.");

    //     return this->follows[fst - 1]->after.count(snd) > 0;
    // }

#if 0
    Follows* ProgramKB::getFollows(simple::ast::StatementNum fst)
    {
        if(fst > this->follows.size() || fst < 1)
            throw util::PkbException("pkb::eval", "StatementNum out of range.");
        return this->follows[fst - 1];
    }

    // Takes in two 1-indexed StatementNums. Allows for 0 to be used as a wildcard on 1 of the parameters.
    std::unordered_set<s_ast::StatementNum> ProgramKB::getFollowsTList(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        // note: no need to check for < 0 since StatementNum is unsigned
        if(fst > this->follows.size() || snd > this->follows.size())
            throw util::PkbException("pkb::eval", "StatementNum out of range.");

        if((fst < 1 && snd < 1) || (fst != 0 && snd != 0))
            throw util::PkbException("pkb::eval", "Only 1 wildcard is to be used.");

        if(fst == 0)
        {
            return this->follows[snd - 1]->before;
        }
        else if(snd == 0)
        {
            return this->follows[fst - 1]->after;
        }

        throw util::PkbException("pkb", "unreachable code reached");
    }
#endif

    /**
     * Start of Parent methods
     */
    bool ProgramKB::isParent(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(_direct_parents.count(snd) == 0)
            return false;

        return _direct_parents[snd] == fst;
    }

    bool ProgramKB::isParentT(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(_ancestors.count(snd) == 0)
            return false;

        return _ancestors[snd].count(fst);
    }

    std::optional<s_ast::StatementNum> ProgramKB::getParentOf(s_ast::StatementNum fst)
    {
        // this will return 0 if it has no parent
        if(_direct_parents.count(fst) == 0)
            return std::nullopt;

        return _direct_parents[fst];
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getAncestorsOf(s_ast::StatementNum fst)
    {
        if(_ancestors.count(fst) == 0)
            return {};

        return _ancestors[fst];
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getChildrenOf(s_ast::StatementNum fst)
    {
        if(_direct_children.count(fst) == 0)
            return {};

        return _direct_children[fst];
    }

    std::unordered_set<s_ast::StatementNum> ProgramKB::getDescendantsOf(s_ast::StatementNum fst)
    {
        if(_descendants.count(fst) == 0)
            return {};

        return _descendants[fst];
    }

    bool ProgramKB::followsRelationExists()
    {
        return this->m_follows_exists;
    }

    bool ProgramKB::parentRelationExists()
    {
        return this->m_parent_exists;
    }
    /**
     * End of Parent methods
     */

    std::unordered_set<std::string> ProgramKB::getConstants()
    {
        return _constants;
    }

    /**
     * Start of Uses and Modifies methods
     */
    bool ProgramKB::isUses(const simple::ast::StatementNum& stmt_num, const std::string& var)
    {
        if(m_variables.find(var) == m_variables.end() || stmt_num > m_statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        const auto& stmt = m_statements.at(stmt_num - 1);
        if(auto c = dynamic_cast<const s_ast::ProcCall*>(stmt.getAstStmt()))
        {
            return m_procedures[c->proc_name].uses.count(var) > 0;
        }
        else
        {
            return stmt.usesVariable(var);
        }
    }

    bool ProgramKB::isUses(const std::string& proc, const std::string& var)
    {
        if(m_variables.find(var) == m_variables.end() || m_procedures.find(proc) == m_procedures.end())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        return m_procedures.at(proc).uses.count(var) > 0;
    }

    std::unordered_set<std::string> ProgramKB::getUsesVars(const std::string& var)
    {
        if(m_procedures.find(var) == m_procedures.end())
        {
            throw util::PkbException("pkb::eval", "Procedure not found.");
        }
        return m_procedures.at(var).uses;
    }

    std::unordered_set<std::string> ProgramKB::getUses(const pql::ast::DESIGN_ENT& type, const std::string& var)
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

    bool ProgramKB::isModifies(const simple::ast::StatementNum& stmt_num, const std::string& var)
    {
        if(m_variables.find(var) == m_variables.end() || stmt_num > m_statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        auto& stmt = m_statements.at(stmt_num - 1);
        if(auto c = dynamic_cast<const s_ast::ProcCall*>(stmt.getAstStmt()))
        {
            return m_procedures[c->proc_name].modifies.count(var) > 0;
        }
        else
        {
            // return stmt.modifies.count(var) > 0;
            return stmt.modifiesVariable(var);
        }
    }

    bool ProgramKB::isModifies(const std::string& proc, const std::string& var)
    {
        if(m_variables.find(var) == m_variables.end() || m_procedures.find(proc) == m_procedures.end())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        return m_procedures.at(proc).modifies.count(var) > 0;
    }

    std::unordered_set<std::string> ProgramKB::getModifiesVars(const std::string& var)
    {
        if(m_procedures.find(var) == m_procedures.end())
        {
            throw util::PkbException("pkb::eval", "Procedure {} not found.", var);
        }
        return m_procedures.at(var).modifies;
    }

    std::unordered_set<std::string> ProgramKB::getModifies(const pql::ast::DESIGN_ENT& type, const std::string& var)
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
