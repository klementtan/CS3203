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

    void WithCond::evaluate(const pkb::ProgramKB* pkb, eval::table::Table* tbl) const
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

                auto domain = tbl->getDomain(l_decl);
                for(auto it = domain.begin(); it != domain.end();)
                {
                    if(std::to_string(it->getStmtNum()) == num)
                        ++it;
                    else
                        it = domain.erase(it);
                }

                tbl->putDomain(l_decl, domain);
            }
            else if(right->isDeclaration())
            {
                // again, this should be true.
                auto r_decl = right->declaration();
                assert(r_decl->design_ent == ast::DESIGN_ENT::PROG_LINE);

                auto l_domain = tbl->getDomain(l_decl);
                auto r_domain = tbl->getDomain(r_decl);

                // while we would in the ideal case like to just perform a simple set
                // intersection (since all we really need is to ensure the values are
                // the same), this is not possible since an Entry is tied to a particular
                // Declaration, so Entries with the same value (but different declarations,
                // as in this case) will hash to different values. however, we can still
                // make this an O(n) algorithm.

                // note that this does not require joins, since we only need the values
                // to be exactly the same (and not if A == X then B == Y)

                // in this case, we know that both sides are PROG_LINE, so they will be statement
                // numbers. we do not make a distinction between prog_line and stmt.
                std::unordered_set<pkb::StatementNum> l_nums {};
                std::unordered_set<pkb::StatementNum> r_nums {};

                std::for_each(l_domain.begin(), l_domain.end(), [&](auto& v) { l_nums.insert(v.getStmtNum()); });
                std::for_each(r_domain.begin(), r_domain.end(), [&](auto& v) { r_nums.insert(v.getStmtNum()); });

                auto common_nums = table::setIntersction(l_nums, r_nums);

                auto make_new_domain = [&common_nums](ast::Declaration* decl) -> auto {
                    table::Domain ret {};
                    for(auto& num : common_nums)
                        ret.emplace(decl, num);
                    return ret;
                };

                tbl->putDomain(l_decl, make_new_domain(l_decl));
                tbl->putDomain(r_decl, make_new_domain(r_decl));
            }
            else if(right->isAttrRef())
            {
                // this is a little more complicated
            }
            else
            {
                throw PqlException("pql::eval", "unreachable");
            }
        }
        else if(left->isAttrRef())
        {
            auto l_ref = left->attrRef();
            auto l_domain = tbl->getDomain(l_ref.decl);

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

                tbl->putDomain(l_ref.decl, l_domain);
            }
            else if(right->isAttrRef())
            {
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
