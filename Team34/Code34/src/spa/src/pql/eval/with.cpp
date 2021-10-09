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

            else if(attr == AttrName::kStmtNum || attr == AttrName::kValue)
                return TYPE_NUMBER;
        }

        // we shouldn't get here tbh
        throw PqlException("pql::eval", "invalid WithCondRef '{}'", ref.toString());
    }

    void WithCond::evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const
    {
        using Table = eval::table::Table;
        using Entry = eval::table::Entry;

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
            // this should always be true. (many prior places verify this)
            auto l_decl = left->declaration();
            assert(l_decl->design_ent == ast::DESIGN_ENT::PROG_LINE);

            if(right->isNumber())
            {
                auto num = right->stringOrNumber();

                auto domain = table->getDomain(l_decl);
                for(auto it = domain.begin(); it != domain.end();)
                {
                    if(std::to_string(it->getStmtNum()) == num)
                        ++it;
                    else
                        it = domain.erase(it);
                }

                table->putDomain(l_decl, domain);
            }
            else if(right->isDeclaration())
            {
                // again, this should be true.
                auto r_decl = right->declaration();
                assert(r_decl->design_ent == ast::DESIGN_ENT::PROG_LINE);

                // in this case, all we need to do is perform a set intersection,
                // since the only requirement is that they are equal value-wise.
                auto l_domain = table->getDomain(l_decl);
                auto r_domain = table->getDomain(r_decl);

                auto new_domain = eval::table::entry_set_intersect(l_domain, r_domain);
                table->putDomain(l_decl, new_domain);
                table->putDomain(r_decl, new_domain);
            }
            else if(right->isAttrRef())
            {
            }
            else
            {
                throw PqlException("pql::eval", "unreachable");
            }
        }
        else if(left->isAttrRef())
        {
            auto l_ref = left->attrRef();
            auto l_domain = table->getDomain(l_ref.decl);

            if(right->isNumber() || right->isString())
            {
                for(auto it = l_domain.begin(); it != l_domain.end();)
                {
                    auto attr = Table::extractAttr(*it, l_ref, pkb);

                    // note: string and number return the same field.
                    if(attr.getVal() != right->stringOrNumber())
                        it = l_domain.erase(it);
                    else
                        ++it;
                }

                table->putDomain(l_ref.decl, l_domain);
            }
            else
            {
            #if 0
                // because unordered_set returns const iterators because it sucks, we are forced
                // to construct a new set.
                eval::table::Domain new_l_domain {};
                std::transform(l_domain.begin(), l_domain.end(), std::inserter(new_l_domain, new_l_domain.begin()),
                    [&](auto& entry) { return Table::extractAttr(entry, l_ref, pkb); });

                eval::table::Domain r_domain {};
                if(right->isDeclaration())
                {
                    r_domain = table->getDomain(right->declaration());
                }
                else
                {

                }

                auto r_domain = right->isDeclaration()
                    ? table->getDomain(right->declaration())
                    : table->getDomain(right->attrRef().decl);

                eval::table::Domain new_r_domain {};
                std::transform(r_domain.begin(), r_domain.end(), std::inserter(new_r_domain, new_r_domain.begin()),
                    [&](auto& entry) { return Table::extractAttr(entry, r_ref, pkb); });
            #endif
            }
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }
}
