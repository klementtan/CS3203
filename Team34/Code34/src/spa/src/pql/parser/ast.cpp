// pqlast.cpp

#include <cassert>
#include <algorithm>

#include <zpr.h>

#include "exceptions.h"
#include "pql/parser/ast.h"

namespace pql::ast
{
    Clause::~Clause() { }

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
        { "prog_line", DESIGN_ENT::PROG_LINE },
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
        { DESIGN_ENT::PROG_LINE, "prog_line" },

        { DESIGN_ENT::INVALID, "invalid" },
    };

    const std::unordered_map<std::string, AttrName> AttrNameMap = {
        { "procName", AttrName::kProcName },
        { "varName", AttrName::kVarName },
        { "value", AttrName::kValue },
        { "stmt#", AttrName::kStmtNum },
    };

    const std::unordered_map<AttrName, std::string> InvAttrNameMap = {
        { AttrName::kProcName, "procName" },
        { AttrName::kVarName, "varName" },
        { AttrName::kValue, "value" },
        { AttrName::kStmtNum, "stmt#" },
        { AttrName::kInvalid, "invalid" },
    };

    bool DeclarationList::hasDeclaration(const std::string& name) const
    {
        return this->declarations.find(name) != this->declarations.end();
    }

    Declaration* DeclarationList::getDeclaration(const std::string& name) const
    {
        if(auto it = this->declarations.find(name); it != this->declarations.end())
            return it->second;

        return nullptr;
    }

    Declaration* DeclarationList::addDeclaration(const std::string& name, DESIGN_ENT design_ent)
    {
        assert(!this->hasDeclaration(name));

        return this->declarations.emplace(name, new Declaration { name, design_ent }).first->second;
    }

    const std::unordered_map<std::string, Declaration*>& DeclarationList::getAllDeclarations() const
    {
        return this->declarations;
    }

    bool Declaration::operator==(const Declaration& other) const
    {
        return this->name == other.name && this->design_ent == other.design_ent;
    }


    Declaration* StmtRef::declaration() const
    {
        if(this->ref_type != Type::Declaration)
            throw util::PqlException("pql", "StmtRef is not a Declaration");

        return this->_declaration;
    }

    simple::ast::StatementNum StmtRef::id() const
    {
        if(this->ref_type != Type::StmtId)
            throw util::PqlException("pql", "StmtRef is not a StmtId");

        return this->_id;
    }


    StmtRef StmtRef::ofWildcard()
    {
        StmtRef ret {};
        ret.ref_type = Type::Wildcard;
        return ret;
    }

    StmtRef StmtRef::ofDeclaration(Declaration* decl)
    {
        StmtRef ret {};
        ret.ref_type = Type::Declaration;
        ret._declaration = decl;
        return ret;
    }

    StmtRef StmtRef::ofStatementId(simple::ast::StatementNum id)
    {
        StmtRef ret {};
        ret.ref_type = Type::StmtId;
        ret._id = id;
        return ret;
    }

    Declaration* EntRef::declaration() const
    {
        if(this->ref_type != Type::Declaration)
            throw util::PqlException("pql", "EntRef is not a Declaration");

        return this->_declaration;
    }

    std::string EntRef::name() const
    {
        if(this->ref_type != Type::Name)
            throw util::PqlException("pql", "EntRef is not a Name");

        return this->_name;
    }


    EntRef EntRef::ofWildcard()
    {
        EntRef ret {};
        ret.ref_type = Type::Wildcard;
        return ret;
    }

    EntRef EntRef::ofDeclaration(Declaration* decl)
    {
        EntRef ret {};
        ret.ref_type = Type::Declaration;
        ret._declaration = decl;
        return ret;
    }

    EntRef EntRef::ofName(std::string name)
    {
        EntRef ret {};
        ret.ref_type = Type::Name;
        ret._name = std::move(name);
        return ret;
    }

    Declaration* Elem::declaration() const
    {
        if(this->ref_type != Type::Declaration)
            throw util::PqlException("pql", "Elem is not a Declaration");

        return this->_declaration;
    }


    AttrRef Elem::attrRef() const
    {
        if(this->ref_type != Type::AttrRef)
            throw util::PqlException("pql", "Elem is not a AttrRef");

        return this->_attr_ref;
    }

    Elem Elem::ofAttrRef(AttrRef attr_ref)
    {
        Elem ret {};
        ret.ref_type = Type::AttrRef;
        ret._attr_ref = attr_ref;
        return ret;
    }

    Elem Elem::ofDeclaration(Declaration* decl)
    {
        Elem ret {};
        ret.ref_type = Type::Declaration;
        ret._declaration = decl;
        return ret;
    }

    ResultCl ResultCl::ofBool()
    {
        ResultCl ret {};
        ret.type = Type::Bool;
        return ret;
    };

    ResultCl ResultCl::ofTuple(const std::vector<Elem>& tuple)
    {
        ResultCl ret {};
        ret.type = Type::Tuple;
        ret._tuple = std::move(tuple);
        return ret;
    };

    std::vector<Elem> ResultCl::tuple() const
    {
        if(this->type != Type::Tuple)
        {
            throw util::PqlException("pql::ast", "Cannot get tuple from non-tuple type ResultCl.");
        }
        return _tuple;
    }

    std::string WithCondRef::str() const
    {
        if(m_type != Type::String)
            throw util::PqlException("pql", "WithCondRef is not a string");

        return this->_string_or_number;
    }

    std::string WithCondRef::number() const
    {
        if(m_type != Type::Number)
            throw util::PqlException("pql", "WithCondRef is not an integer");

        return this->_string_or_number;
    }

    AttrRef WithCondRef::attrRef() const
    {
        if(m_type != Type::AttrRef)
            throw util::PqlException("pql", "WithCondRef is not an AttrRef");

        return this->_attr_ref;
    }

    Declaration* WithCondRef::declaration() const
    {
        if(m_type != Type::Declaration)
            throw util::PqlException("pql", "WithCondRef is not a declaration");

        return this->_decl;
    }

    WithCondRef WithCondRef::ofString(std::string s)
    {
        WithCondRef wcr {};
        wcr.m_type = Type::String;
        wcr._string_or_number = std::move(s);
        return wcr;
    }

    WithCondRef WithCondRef::ofNumber(std::string i)
    {
        WithCondRef wcr {};
        wcr.m_type = Type::Number;
        wcr._string_or_number = std::move(i);
        return wcr;
    }

    WithCondRef WithCondRef::ofAttrRef(AttrRef a)
    {
        WithCondRef wcr {};
        wcr.m_type = Type::AttrRef;
        wcr._attr_ref = std::move(a);
        return wcr;
    }

    WithCondRef WithCondRef::ofDeclaration(Declaration* d)
    {
        WithCondRef wcr {};
        wcr.m_type = Type::Declaration;
        wcr._decl = d;
        return wcr;
    }
}
