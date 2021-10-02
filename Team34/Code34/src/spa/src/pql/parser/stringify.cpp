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
        return zpr::sprint("ModifiesS(modifier:{}, ent:{})", this->modifier.toString(), this->ent.toString());
    }

    std::string ModifiesP::toString() const
    {
        return zpr::sprint("ModifiesP(modifier:{}, ent:{})", this->modifier.toString(), this->ent.toString());
    }

    std::string UsesS::toString() const
    {
        return zpr::sprint("UsesS(user: {}, ent:{})", this->user.toString(), this->ent.toString());
    }

    std::string UsesP::toString() const
    {
        return zpr::sprint("UsesP(user: {}, ent:{})", this->user.toString(), this->ent.toString());
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

    std::string Calls::toString() const
    {
        return zpr::sprint("Calls(caller:{}, proc:{})", this->caller.toString(), this->proc.toString());
    }

    std::string CallsT::toString() const
    {
        return zpr::sprint("CallsT(caller:{}, proc:{})", this->caller.toString(), this->proc.toString());
    }

    std::string ExprSpec::toString() const
    {
        return zpr::sprint(
            "ExprSpec(is_subexpr:{}, expr:{})", this->is_subexpr, this->expr ? this->expr->toString() : "nullptr");
    }

    std::string AssignPatternCond::toString() const
    {
        return zpr::sprint("AssignPatternCl(ent:{}, assignment_declaration:{}, expr_spec:{})", this->ent.toString(),
            this->assignment_declaration ? this->assignment_declaration->toString() : "nullptr",
            this->expr_spec.toString());
    }

    std::string IfPatternCond::toString() const
    {
        return zpr::sprint("IfPatternCl(ent:{}, if_declaration:{})", this->ent.toString(),
            this->if_declaration ? this->if_declaration->toString() : "nullptr");
    }

    std::string WhilePatternCond::toString() const
    {
        return zpr::sprint("WhilePatternCl(ent:{}, if_declaration:{})", this->ent.toString(),
            this->while_declaration ? this->while_declaration->toString() : "nullptr");
    }

    std::string WithCondRef::toString() const
    {
        if(this->isString())
            return zpr::sprint("WithCondRef(string: '{}')", this->str());
        else if(this->isInteger())
            return zpr::sprint("WithCondRef(int: '{}')", this->integer());
        else if(this->isDeclaration())
            return zpr::sprint("WithCondRef(decl: {})", this->declaration()->toString());
        else if(this->isAttrRef())
            return zpr::sprint("WithCondRef(attrRef: {})", this->attrRef().toString());
        else
            return zpr::sprint("WithCondRef(???)");
    }

    std::string WithCond::toString() const
    {
        return zpr::sprint("WithCond(lhs:{}, rhs:{})", this->lhs.toString(), this->rhs.toString());
    }


    template <typename T>
    static std::string list_to_string(const std::vector<std::unique_ptr<T>>& list)
    {
        std::string ret = "[\n";
        for(const auto& x : list)
            ret += zpr::sprint("\t{}\n", x.get()->toString());
        return ret + "]";
    }

    std::string Select::toString() const
    {
        return zpr::sprint("Select(relations:{}, patterns:{}, withs:{}, result:{})", list_to_string(this->relations),
            list_to_string(this->patterns), list_to_string(this->withs), this->result.toString());
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

    std::string AttrRef::toString() const
    {
        auto it = InvAttrNameMap.find(this->attr_name);
        std::string attr_name = it == InvAttrNameMap.end() ? "invalid" : it->second;
        return zpr::sprint("AttrRef(decl: {}, attr_name: {})", decl ? decl->toString() : "nullptr", attr_name);
    }

    std::string Elem::toString() const
    {
        std::string ret = zpr::sprint("Elem(ref_type: ");
        switch(this->ref_type)
        {
            case Type::Invalid:
                ret += "Invalid";
                break;
            case Type::AttrRef:
                ret += zpr::sprint("AttrRef, attr_ref: {}", _attr_ref.toString());
                break;
            case Type::Declaration:
                assert(_declaration);
                ret += zpr::sprint("Declaration, decl: {}", _declaration->toString());
                break;
        }
        ret += ")";
        return ret;
    }

    std::string ResultCl::toString() const
    {
        if(this->type == Type::Bool)
        {
            return "ResultCl(type: Bool)";
        }
        else if(this->type == Type::Tuple)
        {
            std::string ret { "ResultCl(type: Tuple, tuple :[" };
            for(const Elem& elem : _tuple)
            {
                ret += zpr::sprint("{}", elem.toString());
            }
            ret += "]";
            return ret;
        }
        else
        {
            return "ResultCl(type: Invalid)";
        }
    }
}
