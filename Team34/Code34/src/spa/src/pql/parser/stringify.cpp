// stringify.cpp

#include <cassert>
#include <algorithm>

#include <zpr.h>

#include "exceptions.h"
#include "pql/parser/ast.h"

namespace pql::ast
{
    std::string DeclarationList::toString() const
    {
        std::string ret { "DeclarationList[\n" };

        // for the sake of testing (since toString() is really only used in testing), we
        // need a consistent ordering of all these unordered_maps...
        std::vector<std::pair<std::string, const Declaration*>> decls;
        for(const auto& [name, decl] : this->declarations)
            decls.emplace_back(name, decl);

        std::sort(decls.begin(), decls.end(), [](auto& a, auto& b) -> bool { return a.first < b.first; });

        for(const auto& [name, decl] : decls)
        {
            ret += zpr::sprint("\tname:{}, declaration:{}\n", name, decl ? decl->toString() : "nullptr");
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

    std::string ModifiesS::toString() const
    {
        return zpr::sprint(
            "ModifiesS(modifier:{}, ent:{})", this->modifier.toString(), this->ent.toString());
    }

    std::string ModifiesP::toString() const
    {
        return zpr::sprint("ModifiesP(modifier:{}, ent:{})", this->modifier.toString(),
            this->ent.toString());
    }

    std::string UsesS::toString() const
    {
        return zpr::sprint(
            "UsesS(user: {}, ent:{})", this->user.toString(), this->ent.toString());
    }

    std::string UsesP::toString() const
    {
        return zpr::sprint(
            "UsesP(user: {}, ent:{})", this->user.toString(), this->ent.toString());
    }

    std::string Parent::toString() const
    {
        return zpr::sprint("Parent(parent:{}, child:{})", this->parent.toString(), this->child.toString());
    }

    std::string ParentT::toString() const
    {
        return zpr::sprint(
            "ParentT(ancestor:{}, descendant:{})", this->ancestor.toString(), this->descendant.toString());
    }

    std::string Follows::toString() const
    {
        return zpr::sprint("Follows(directly_before:{}, directly_after:{})", this->directly_before.toString(),
            this->directly_after.toString());
    }

    std::string FollowsT::toString() const
    {
        return zpr::sprint("FollowsT(before:{}, after{})", this->before.toString(), this->after.toString());
    }

    std::string ExprSpec::toString() const
    {
        return zpr::sprint(
            "ExprSpec(is_subexpr:{}, expr:{})", this->is_subexpr, this->expr ? this->expr->toString() : "nullptr");
    }

    std::string AssignPatternCond::toString() const
    {
        return zpr::sprint("PatternCl(ent:{}, assignment_declaration:{}, expr_spec:{})", this->ent.toString(),
            this->assignment_declaration ? this->assignment_declaration->toString() : "nullptr",
            this->expr_spec.toString());
    }

    std::string PatternCl::toString() const
    {
        std::string ret { "PatternCl[\n" };
        for(const auto& pattern_cond : this->pattern_conds)
            ret += zpr::sprint("\t{}\n", pattern_cond ? pattern_cond->toString() : "nullptr");

        ret += "]";
        return ret;
    }

    std::string SuchThatCl::toString() const
    {
        std::string ret { "SuchThatCl[\n" };
        for(const auto& rel_cond : this->rel_conds)
            ret += zpr::sprint("\t{}\n", rel_cond ? rel_cond->toString() : "nullptr");

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
        return zpr::sprint("Query(select:{}, declarations:{})", this->select.toString(), this->declarations.toString());
    }


    std::string StmtRef::toString() const
    {
        switch(this->ref_type)
        {
            case Type::Declaration:
                return zpr::sprint(
                    "DeclaredStmt(declaration: {})", this->_declaration ? this->_declaration->toString() : "nullptr");

            case Type::StmtId:
                return zpr::sprint("StmtId(id:{})", this->_id);

            case Type::Wildcard:
                return zpr::sprint("AllStmt(name: _)");

            case Type::Invalid:
            default:
                throw util::PqlException("pql", "invalid StmtRef type");
        }
    }

    std::string EntRef::toString() const
    {
        switch(this->ref_type)
        {
            case Type::Declaration:
                return zpr::sprint(
                    "DeclaredEnt(declaration:{})", this->_declaration ? this->_declaration->toString() : "nullptr");

            case Type::Name:
                return zpr::sprint("EntName(name:{})", this->_name);

            case Type::Wildcard:
                return zpr::sprint("AllEnt(name: _)");

            case Type::Invalid:
            default:
                throw util::PqlException("pql", "invalid EntRef type");
        }
    }
}
