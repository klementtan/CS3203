// with.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"


namespace pql::ast
{
    namespace s_ast = simple::ast;
    namespace table = pql::eval::table;

    using PqlException = util::PqlException;

    static constexpr int TYPE_NUMBER = 1;
    static constexpr int TYPE_STRING = 2;

    static int get_type(const WithCondRef& ref)
    {
        if(ref.isNumber())
        {
            return TYPE_NUMBER;
        }
        else if(ref.isString())
        {
            return TYPE_STRING;
        }
        else if(ref.isDeclaration())
        {
            if(ref.declaration()->design_ent != ast::DESIGN_ENT::PROG_LINE)
                throw PqlException("pql::eval", "standalone synonym in 'with' can only be 'prog_line'");

            return TYPE_NUMBER;
        }
        else if(ref.isAttrRef())
        {
            auto ar = ref.attrRef();
            auto attr = ar.attr_name;
            if(attr == AttrName::kProcName || attr == AttrName::kVarName)
                return TYPE_STRING;

            // this is a semantic check, so return TYPE_NUMBER for kValue even though
            // constants are represented as strings in the ast.
            else if(attr == AttrName::kStmtNum || attr == AttrName::kValue)
                return TYPE_NUMBER;
        }

        // we shouldn't get here tbh
        throw PqlException("pql::eval", "invalid WithCondRef '{}'", ref.toString());
    }

    using Join = table::Join;
    using Table = table::Table;
    using Entry = table::Entry;
    using Domain = table::Domain;

    static Entry replace_entry_decl(const Entry& ent, Declaration* new_decl)
    {
        auto ent_de = ent.getDeclaration()->design_ent;
        auto new_de = new_decl->design_ent;

        if(new_de == DESIGN_ENT::CONSTANT)
        {
            if(ent_de != DESIGN_ENT::CONSTANT)
                return Entry(new_decl, std::to_string(ent.getStmtNum()));
            else
                return Entry(new_decl, ent.getVal());
        }
        else
        {
            if(ent_de == DESIGN_ENT::PROCEDURE || ent_de == DESIGN_ENT::VARIABLE)
                return Entry(new_decl, ent.getVal());
            else if(ent_de == DESIGN_ENT::CONSTANT)
                return Entry(new_decl, std::stoll(ent.getVal()));
            else
                return Entry(new_decl, ent.getStmtNum());
        }
    }



    void WithCond::evaluate(const pkb::ProgramKB* pkb, Table* tbl) const
    {
        if(get_type(this->lhs) != get_type(this->rhs))
            throw PqlException("pql::eval", "incompatible types for two sides of 'with'");

        // note: get_type already ensures that we only do str = str or num = num.
        if((this->lhs.isString() || this->lhs.isNumber()) && (this->rhs.isString() || this->rhs.isNumber()))
        {
            if(this->lhs.stringOrNumber() != this->rhs.stringOrNumber())
                throw PqlException(
                    "pql::eval", "'{}' = '{}' is always false", this->lhs.stringOrNumber(), this->rhs.stringOrNumber());

            // always true
            return;
        }

        auto* left = &this->lhs;
        auto* right = &this->rhs;

        // make it so the left side is either a decl or an attrref.
        if(left->isNumber() || left->isString() || (left->isAttrRef() && right->isDeclaration()))
            std::swap(left, right);

        assert(!left->isNumber() && !left->isString());


        if(left->isDeclaration())
        {
            auto l_decl = left->declaration();
            tbl->addSelectDecl(l_decl);

            // this should always be true. (many prior places verify this)
            assert(l_decl->design_ent == DESIGN_ENT::PROG_LINE);


            if(right->isNumber())
            {
                auto num = right->stringOrNumber();

                auto domain = tbl->getDomain(l_decl);
                for(auto it = domain.begin(); it != domain.end();)
                {
                    // number literals in the with clause are stored as strings (because they may be
                    // arbitrarily long), so convert the stmtnum (actually a prog_line) to string.
                    if(std::to_string(it->getStmtNum()) == num)
                        ++it;
                    else
                        it = domain.erase(it);
                }

                tbl->putDomain(l_decl, domain);
            }
            else
            {
                auto r_decl = right->isDeclaration() ? right->declaration() : right->attrRef().decl;
                tbl->addSelectDecl(r_decl);

                auto l_domain = tbl->getDomain(l_decl);
                auto r_domain = tbl->getDomain(r_decl);

                std::unordered_set<std::pair<Entry, Entry>> join_pairs {};

                // loop over the smaller set.
                if(r_domain.size() < l_domain.size())
                {
                    std::swap(l_domain, r_domain);
                    std::swap(l_decl, r_decl);
                }

                for(auto& entry : l_domain)
                    join_pairs.emplace(entry, replace_entry_decl(entry, r_decl));

                tbl->addJoin(Join(l_decl, r_decl, std::move(join_pairs)));
            }
        }
        else if(left->isAttrRef())
        {
            auto l_ref = left->attrRef();
            auto l_decl = l_ref.decl;
            auto l_domain = tbl->getDomain(l_ref.decl);

            tbl->addSelectDecl(l_decl);

            if(right->isNumber() || right->isString())
            {
                for(auto it = l_domain.begin(); it != l_domain.end();)
                {
                    auto attr = Table::extractAttr(*it, l_ref, pkb);

                    bool equals = false;
                    if(l_ref.attr_name == AttrName::kValue)
                    {
                        assert(right->isNumber());
                        equals = right->stringOrNumber() == attr.getVal();
                    }
                    else if(l_ref.attr_name == AttrName::kStmtNum)
                    {
                        // TODO: consider changing this to convert the string to a number, since
                        // (i think) that is potentially faster.
                        assert(right->isNumber());
                        equals = right->stringOrNumber() == std::to_string(attr.getStmtNum());
                    }
                    else
                    {
                        assert(l_ref.attr_name == AttrName::kProcName || l_ref.attr_name == AttrName::kVarName);
                        equals = right->stringOrNumber() == attr.getVal();
                    }

                    if(!equals)
                        it = l_domain.erase(it);
                    else
                        ++it;
                }

                tbl->putDomain(l_decl, l_domain);
            }
            else if(right->isAttrRef())
            {
                auto r_decl = right->attrRef().decl;
                auto r_domain = tbl->getDomain(r_decl);
                tbl->addSelectDecl(r_decl);

                std::unordered_set<std::pair<Entry, Entry>> join_pairs {};

                // loop over the smaller set.
                if(r_domain.size() < l_domain.size())
                {
                    std::swap(l_domain, r_domain);
                    std::swap(l_decl, r_decl);
                }

                for(auto& entry : l_domain)
                    join_pairs.emplace(entry, replace_entry_decl(entry, r_decl));

                tbl->addJoin(Join(l_decl, r_decl, std::move(join_pairs)));
            }
            else
            {
                // right cannot be a decl because (attrRef, decl) would've been swapped to
                // (decl, attrRef) above.
                throw PqlException("pql::eval", "unreachable");
            }
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }
}
