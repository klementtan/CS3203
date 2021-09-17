// pqlast.cpp

#include <cassert>
#include <algorithm>

#include <zpr.h>

#include "exceptions.h"
#include "pql/parser/ast.h"

namespace pql::ast
{
    EntRef::~EntRef() { }

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

    void DeclarationList::addDeclaration(std::string name, Declaration* decl)
    {
        assert(!this->hasDeclaration(name));

        this->declarations.emplace(std::move(name), decl);
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
}
