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

    using Table = table::Table;
    using Entry = table::Entry;
    using Domain = table::Domain;

    template <typename T>
    static Domain make_new_domain(ast::Declaration* decl, const std::unordered_set<T>& values)
    {
        Domain ret {};

        // only constants store numbers as strings. the rest (stmt and prog_line)
        // store them as numbers.
        if(decl->design_ent == ast::DESIGN_ENT::CONSTANT)
        {
            for(auto& val : values)
            {
                if constexpr(std::is_same_v<T, std::string>)
                    ret.emplace(decl, val);
                else
                    ret.emplace(decl, std::to_string(val));
            }
        }
        else
        {
            for(auto& val : values)
                ret.emplace(decl, val);
        }

        return ret;
    }

    static void get_numbers_from_domain(const Domain& domain, const AttrRef& ref, std::unordered_set<size_t>* out)
    {
        if(ref.attr_name == AttrName::kValue)
        {
            std::for_each(domain.begin(), domain.end(), [&](auto& v) {
                try
                {
                    out->insert(std::stoll(v.getVal()));
                }
                catch(const std::out_of_range& e)
                {
                    // it doesn't matter. the intersection will never include this number, since
                    // no stmt num can reach this value. so, there's no point thinking about
                    // inserting it into the domain.
                }
            });
        }
        else
        {
            assert(ref.attr_name == AttrName::kStmtNum);
            std::for_each(domain.begin(), domain.end(), [&](auto& v) { out->insert(v.getStmtNum()); });
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
            // this should always be true. (many prior places verify this)
            auto l_decl = left->declaration();
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
                // in this case, we know that both sides are numbers. if the right
                // side is an attrRef, it has to be .stmt# or .value. for the constant value, they
                // are stored as strings, but it is cheaper to do string -> int than vice-versa.
                std::unordered_set<size_t> l_nums {};
                std::unordered_set<size_t> r_nums {};

                auto l_domain = tbl->getDomain(l_decl);
                std::for_each(l_domain.begin(), l_domain.end(), [&](auto& v) { l_nums.insert(v.getStmtNum()); });

                Declaration* r_decl = nullptr;


                if(right->isDeclaration())
                {
                    // again, this should be true.
                    r_decl = right->declaration();
                    assert(r_decl->design_ent == DESIGN_ENT::PROG_LINE);

                    auto r_domain = tbl->getDomain(r_decl);
                    std::for_each(r_domain.begin(), r_domain.end(), [&](auto& v) { r_nums.insert(v.getStmtNum()); });
                }
                else if(right->isAttrRef())
                {
                    auto rref = right->attrRef();
                    r_decl = rref.decl;

                    auto r_domain = tbl->getDomain(r_decl);
                    get_numbers_from_domain(r_domain, rref, &r_nums);
                }
                else
                {
                    throw PqlException("pql::eval", "unreachable");
                }

                // while we would in the ideal case like to just perform a simple set
                // intersection (since all we really need is to ensure the values are
                // the same), this is not possible since an Entry is tied to a particular
                // Declaration, so Entries with the same value (but different declarations,
                // as in this case) will hash to different values. however, we can still
                // make this an O(n) algorithm.

                // note that this does not require joins, since we only need the values
                // to be exactly the same (and not if A == X then B == Y)

                assert(r_decl != nullptr);

                auto common_nums = table::setIntersction(l_nums, r_nums);

                tbl->putDomain(l_decl, make_new_domain(l_decl, common_nums));
                tbl->putDomain(r_decl, make_new_domain(r_decl, common_nums));
            }
        }
        else if(left->isAttrRef())
        {
            auto l_ref = left->attrRef();
            auto l_decl = l_ref.decl;
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

                tbl->putDomain(l_decl, l_domain);
            }
            else if(right->isAttrRef())
            {
                // this is the complex case, mostly because of the different types involved.
                auto r_ref = right->attrRef();
                auto r_decl = r_ref.decl;
                auto r_domain = tbl->getDomain(r_decl);

                auto l_name = l_ref.attr_name;
                auto r_name = r_ref.attr_name;

                // if the left and right are (proc/var) names, then do a string comparison.
                // however, if the left and right are *both* constants, then also do a string comparison,
                // since constants are stored as strings (and we don't want to waste time converting them to ints)
                if(((l_name == AttrName::kProcName || l_name == AttrName::kVarName)
                    && (r_name == AttrName::kProcName || r_name == AttrName::kVarName))
                    || (l_name == AttrName::kValue && r_name == AttrName::kValue))
                {
                    // this is the easy case, honestly.
                    std::unordered_set<std::string> l_strs {};
                    std::unordered_set<std::string> r_strs {};

                    std::for_each(l_domain.begin(), l_domain.end(), [&](auto& v) { l_strs.insert(v.getVal()); });
                    std::for_each(r_domain.begin(), r_domain.end(), [&](auto& v) { r_strs.insert(v.getVal()); });

                    auto common_strs = table::setIntersction(l_strs, r_strs);

                    tbl->putDomain(l_decl, make_new_domain(l_decl, common_strs));
                    tbl->putDomain(r_decl, make_new_domain(r_decl, common_strs));
                }
                else if((l_name == AttrName::kStmtNum || l_name == AttrName::kValue)
                    && (r_name == AttrName::kStmtNum || r_name == AttrName::kValue))
                {
                    std::unordered_set<size_t> l_nums {};
                    std::unordered_set<size_t> r_nums {};

                    get_numbers_from_domain(l_domain, l_ref, &l_nums);
                    get_numbers_from_domain(r_domain, r_ref, &r_nums);

                    auto common_nums = table::setIntersction(l_nums, r_nums);

                    tbl->putDomain(l_decl, make_new_domain(l_decl, common_nums));
                    tbl->putDomain(r_decl, make_new_domain(r_decl, common_nums));
                }
                else
                {
                    throw PqlException("pql::eval", "unreachable");
                }
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
