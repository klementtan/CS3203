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
        if(stmt_no > this->uses_modifies.statements.size())
        {
            throw util::PkbException("pkb", "Statement is out of range");
        }
        return this->uses_modifies.statements.at(stmt_no - 1);
    }

    // Takes in two 1-indexed StatementNums
    bool ProgramKB::isFollows(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        // thinking of more elegant ways of handling this hmm
        if(fst > this->follows.size() || snd > this->follows.size() || fst < 1 || snd < 1)
            throw util::PkbException("pkb::eval", "StatementNum out of range.");

        return this->follows[fst - 1]->directly_after == snd;
    }

    // Same as isFollows
    bool ProgramKB::isFollowsT(s_ast::StatementNum fst, s_ast::StatementNum snd)
    {
        if(fst > this->follows.size() || snd > this->follows.size() || fst < 1 || snd < 1)
            throw util::PkbException("pkb::eval", "StatementNum out of range.");

        return this->follows[fst - 1]->after.count(snd) > 0;
    }
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
    bool UsesModifies::isUses(const simple::ast::StatementNum& stmt_num, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end() || stmt_num > this->statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        const auto& stmt = this->statements.at(stmt_num - 1);
        if(auto c = dynamic_cast<s_ast::ProcCall*>(stmt->stmt))
        {
            return this->procedures[c->proc_name].uses.count(var) > 0;
        }
        else
        {
            return stmt->uses.count(var) > 0;
        }
    }

    bool UsesModifies::isUses(const std::string& proc, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end() || this->procedures.find(proc) == this->procedures.end())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        return this->procedures.at(proc).uses.count(var) > 0;
    }

    std::unordered_set<std::string> UsesModifies::getUsesVars(const simple::ast::StatementNum& stmt_num)
    {
        if(stmt_num > this->statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid statement number.");
        }
        return this->statements.at(stmt_num - 1)->uses;
    }

    std::unordered_set<std::string> UsesModifies::getUsesVars(const std::string& var)
    {
        if(this->procedures.find(var) == this->procedures.end())
        {
            throw util::PkbException("pkb::eval", "Procedure not found.");
        }
        return this->procedures.at(var).uses;
    }

    std::unordered_set<std::string> UsesModifies::getUses(const pql::ast::DESIGN_ENT& type, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end())
        {
            throw util::PkbException("pkb::eval", "Variable not found.");
        }
        std::unordered_set<std::string> uses;
        auto& stmt_list = this->variables.at(var).used_by;
        switch(type)
        {
            case pql::ast::DESIGN_ENT::ASSIGN:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::AssignStmt*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PRINT:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::PrintStmt*>(stmt))
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
                    if(dynamic_cast<s_ast::ProcCall*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::IF:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::IfStmt*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::WHILE:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::WhileLoop*>(stmt))
                        uses.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PROCEDURE:
                for(auto& proc : this->variables.at(var).used_by_procs)
                    uses.insert(proc->name);
                break;
            default:
                throw util::PkbException("pkb::eval", "Invalid statement type.");
        }
        return uses;
    }

    bool UsesModifies::isModifies(const simple::ast::StatementNum& stmt_num, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end() || stmt_num > this->statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        auto& stmt = this->statements.at(stmt_num - 1);
        if(auto c = dynamic_cast<s_ast::ProcCall*>(stmt->stmt))
        {
            return this->procedures[c->proc_name].modifies.count(var) > 0;
        }
        else
        {
            return stmt->modifies.count(var) > 0;
        }
    }

    bool UsesModifies::isModifies(const std::string& proc, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end() || this->procedures.find(proc) == this->procedures.end())
        {
            throw util::PkbException("pkb::eval", "Invalid query parameters.");
        }
        return this->procedures.at(proc).modifies.count(var) > 0;
    }

    std::unordered_set<std::string> UsesModifies::getModifiesVars(const simple::ast::StatementNum& stmt_num)
    {
        if(stmt_num > this->statements.size())
        {
            throw util::PkbException("pkb::eval", "Invalid statement number.");
        }
        return this->statements.at(stmt_num - 1)->modifies;
    }

    std::unordered_set<std::string> UsesModifies::getModifiesVars(const std::string& var)
    {
        if(this->procedures.find(var) == this->procedures.end())
        {
            throw util::PkbException("pkb::eval", "Procedure {} not found.", var);
        }
        return this->procedures.at(var).modifies;
    }

    std::unordered_set<std::string> UsesModifies::getModifies(const pql::ast::DESIGN_ENT& type, const std::string& var)
    {
        if(this->variables.find(var) == this->variables.end())
        {
            throw util::PkbException("pkb::eval", "Variable not found.");
        }
        std::unordered_set<std::string> modifies;
        auto& stmt_list = this->variables.at(var).modified_by;
        switch(type)
        {
            case pql::ast::DESIGN_ENT::ASSIGN:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::AssignStmt*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::READ:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::ReadStmt*>(stmt))
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
                    if(dynamic_cast<s_ast::ProcCall*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::IF:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::IfStmt*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::WHILE:
                for(auto& stmt : stmt_list)
                {
                    if(dynamic_cast<s_ast::WhileLoop*>(stmt))
                        modifies.insert(std::to_string(stmt->id));
                }
                break;
            case pql::ast::DESIGN_ENT::PROCEDURE:
                for(auto& proc : this->variables.at(var).modified_by_procs)
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
}
