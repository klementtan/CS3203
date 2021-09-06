// pqlast.cpp

#include <zpr.h>
#include <zst.h>

#include "pql/ast.h"

namespace pql::ast
{
    EntRef::~EntRef() { }

    StmtRef::~StmtRef() { }

    RelCond::~RelCond() { }

    PatternCond::~PatternCond() { }

    const std::unordered_map<std::string, DESIGN_ENT> DESIGN_ENT_MAP = {
        { "stmt", DESIGN_ENT::STMT },
        { "read", DESIGN_ENT::READ },
        { "print", DESIGN_ENT::PRINT },
        { "call", DESIGN_ENT::WHILE },
        { "if", DESIGN_ENT::IF },
        { "assign", DESIGN_ENT::ASSIGN },
        { "variable", DESIGN_ENT::VARIABLE },
        { "constant", DESIGN_ENT::CONSTANT },
        { "procedure", DESIGN_ENT::PROCEDURE },
    };

    const std::unordered_map<DESIGN_ENT, std::string> INV_DESIGN_ENT_MAP = {
        { DESIGN_ENT::STMT, "stmt" },
        { DESIGN_ENT::READ, "read" },
        { DESIGN_ENT::PRINT, "print" },
        { DESIGN_ENT::WHILE, "call" },
        { DESIGN_ENT::IF, "if" },
        { DESIGN_ENT::ASSIGN, "assign" },
        { DESIGN_ENT::VARIABLE, "variable" },
        { DESIGN_ENT::CONSTANT, "constant" },
        { DESIGN_ENT::PROCEDURE, "procedure" },
    };

    std::string DeclarationList::toString() const
    {
        std::string ret { "DeclarationList[\n" };

        for(const auto& name_declaration : this->declarations)
        {
            ret +=
                zpr::sprint("\tname:{}, declaration:{}\n", name_declaration.first, name_declaration.second->toString());
        }
        ret += "]";
        return ret;
    }

    std::string Declaration::toString() const
    {
        std::string ent_str = INV_DESIGN_ENT_MAP.find(this->design_ent)->second;
        return zpr::sprint("Declaration(ent:{}, name:{})", ent_str, this->name);
    }

    std::string DeclaredStmt::toString() const
    {
        return zpr::sprint("DeclaredStmt(declaration: {})", this->declaration->toString());
    }

    std::string StmtId::toString() const
    {
        return zpr::sprint("StmtId(id:{})", this->id);
    }

    std::string AllStmt::toString() const
    {
        return zpr::sprint("AllStmt(name: _)");
    }

    std::string DeclaredEnt::toString() const
    {
        return zpr::sprint("DeclaredEnt(declaration:{})", this->declaration->toString());
    }

    std::string EntName::toString() const
    {
        return zpr::sprint("EntName(name:{})", this->name);
    }

    std::string AllEnt::toString() const
    {
        return zpr::sprint("AllEnt(name: _)");
    }

    std::string ModifiesS::toString() const
    {
        return zpr::sprint("ModifiesS(modifier:{}, ent:{})", this->modifier->toString(), this->ent->toString());
    }

    std::string UsesS::toString() const
    {
        return zpr::sprint("UsesS(user: {}, ent:{})", this->user->toString(), this->ent->toString());
    }

    std::string Parent::toString() const
    {
        return zpr::sprint("Parent(parent:{}, child:{})", this->parent->toString(), this->child->toString());
    }

    std::string ParentT::toString() const
    {
        return zpr::sprint(
            "ParentT(ancestor:{}, descendant:{})", this->ancestor->toString(), this->descendant->toString());
    }

    std::string Follows::toString() const
    {
        return zpr::sprint("Follows(directly_before:{}, directly_after:{})", this->directly_before->toString(),
            this->directly_after->toString());
    }

    std::string FollowsT::toString() const
    {
        return zpr::sprint("FollowsT(before:{}, after{})", this->before->toString(), this->after->toString());
    }

    std::string ExprSpec::toString() const
    {
        return zpr::sprint(
            "ExprSpec(any_before:{}, any_after:{}, expr:{})", this->any_before, this->any_after, this->expr);
    }

    std::string AssignPatternCond::toString() const
    {
        return zpr::sprint("PatternCl(ent:{}, assignment_declaration:{}, expr_spec:{})", this->ent->toString(),
            this->assignment_declaration->toString(), expr_spec->toString());
    }

    std::string PatternCl::toString() const
    {
        std::string ret { "PatternCl[\n" };
        for(const PatternCond* pattern_cond : this->pattern_conds)
        {
            ret += zpr::sprint("\t{}\n", pattern_cond->toString());
        }
        ret += "]";
        return ret;
    }

    std::string SuchThatCl::toString() const
    {
        std::string ret { "SuchThatCl[\n" };
        for(const RelCond* rel_cond : this->rel_conds)
        {
            ret += zpr::sprint("\t{}\n", rel_cond->toString());
        }
        ret += "]";
        return ret;
    }

    std::string Select::toString() const
    {
        return zpr::sprint("Select(such_that:{}, pattern:{}, ent:{})", this->such_that->toString(),
            this->pattern->toString(), this->ent->toString());
    }

    std::string Query::toString() const
    {
        return zpr::sprint(
            "Query(select:{}, declarations:{})", this->select->toString(), this->declarations->toString());
    }
}
