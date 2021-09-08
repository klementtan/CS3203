// pqlast.cpp

#include <zpr.h>
#include <zst.h>

#include "pql/parser/ast.h"

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
        { "call", DESIGN_ENT::CALL },
        { "while", DESIGN_ENT::WHILE },
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
        { DESIGN_ENT::CALL, "call" },
        { DESIGN_ENT::WHILE, "while" },
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
            ret += zpr::sprint("\tname:{}, declaration:{}\n", name_declaration.first,
                name_declaration.second ? name_declaration.second->toString() : "nullptr");
        }
        ret += "]";
        return ret;
    }

    std::string Declaration::toString() const
    {
        auto it = INV_DESIGN_ENT_MAP.find(this->design_ent);
        if(it == INV_DESIGN_ENT_MAP.end())
        {
            return zpr::sprint("Declaration(nullptr)");
        }
        std::string ent_str = it->second;
        return zpr::sprint("Declaration(ent:{}, name:{})", ent_str, this->name);
    }

    bool Declaration::operator==(const Declaration& other) const
    {
        return this->name == other.name && this->design_ent == other.design_ent;
    }

    std::string DeclaredStmt::toString() const
    {
        return zpr::sprint(
            "DeclaredStmt(declaration: {})", this->declaration ? this->declaration->toString() : "nullptr");
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
        return zpr::sprint(
            "DeclaredEnt(declaration:{})", this->declaration ? this->declaration->toString() : "nullptr");
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
        return zpr::sprint("ModifiesS(modifier:{}, ent:{})", this->modifier ? this->modifier->toString() : "nullptr",
            this->ent ? this->ent->toString() : "nullptr");
    }

    std::string ModifiesP::toString() const
    {
        return zpr::sprint("ModifiesP(modifier:{}, ent:{})", this->modifier ? this->modifier->toString() : "nullptr",
            this->ent ? this->ent->toString() : "nullptr");
    }

    std::string UsesS::toString() const
    {
        return zpr::sprint("UsesS(user: {}, ent:{})", this->user ? this->user->toString() : "nullptr",
            this->ent ? this->ent->toString() : "nullptr");
    }

    std::string UsesP::toString() const
    {
        return zpr::sprint("UsesP(user: {}, ent:{})", this->user ? this->user->toString() : "nullptr",
            this->ent ? this->ent->toString() : "nullptr");
    }

    std::string Parent::toString() const
    {
        return zpr::sprint("Parent(parent:{}, child:{})", this->parent ? this->parent->toString() : "nullptr",
            this->child ? this->child->toString() : "nullptr");
    }

    std::string ParentT::toString() const
    {
        return zpr::sprint("ParentT(ancestor:{}, descendant:{})",
            this->ancestor ? this->ancestor->toString() : "nullptr",
            this->descendant ? this->descendant->toString() : "nullptr");
    }

    std::string Follows::toString() const
    {
        return zpr::sprint("Follows(directly_before:{}, directly_after:{})",
            this->directly_before ? this->directly_before->toString() : "nullptr",
            this->directly_after ? this->directly_after->toString() : "nullptr");
    }

    std::string FollowsT::toString() const
    {
        return zpr::sprint("FollowsT(before:{}, after{})", this->before ? this->before->toString() : "nullptr",
            this->after ? this->after->toString() : "nullptr");
    }

    std::string ExprSpec::toString() const
    {
        return zpr::sprint(
            "ExprSpec(any_before:{}, any_after:{}, expr:{})", this->any_before, this->any_after, this->expr);
    }

    std::string AssignPatternCond::toString() const
    {
        return zpr::sprint("PatternCl(ent:{}, assignment_declaration:{}, expr_spec:{})", this->ent->toString(),
            this->assignment_declaration ? this->assignment_declaration->toString() : "nullptr",
            expr_spec ? this->expr_spec->toString() : "nullptr");
    }

    std::string PatternCl::toString() const
    {
        std::string ret { "PatternCl[\n" };
        for(const PatternCond* pattern_cond : this->pattern_conds)
        {
            ret += zpr::sprint("\t{}\n", pattern_cond ? pattern_cond->toString() : "nullptr");
        }
        ret += "]";
        return ret;
    }

    std::string SuchThatCl::toString() const
    {
        std::string ret { "SuchThatCl[\n" };
        for(const RelCond* rel_cond : this->rel_conds)
        {
            ret += zpr::sprint("\t{}\n", rel_cond ? rel_cond->toString() : "nullptr");
        }
        ret += "]";
        return ret;
    }

    std::string Select::toString() const
    {
        return zpr::sprint("Select(such_that:{}, pattern:{}, ent:{})",
            this->such_that ? this->such_that->toString() : "nullptr",
            this->pattern ? this->pattern->toString() : "nullptr", this->ent ? this->ent->toString() : "nullptr");
    }

    std::string Query::toString() const
    {
        return zpr::sprint("Query(select:{}, declarations:{})", this->select ? this->select->toString() : "nullptr",
            this->declarations ? this->declarations->toString() : "nullptr");
    }
}
