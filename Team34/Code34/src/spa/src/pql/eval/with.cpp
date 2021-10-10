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
            auto l_domain = tbl->getDomain(l_decl);
            tbl->addSelectDecl(l_decl);

            // this should always be true. (many prior places verify this)
            assert(l_decl->design_ent == DESIGN_ENT::PROG_LINE);

            if(right->isNumber())
            {
                auto num = right->stringOrNumber();

                for(auto it = l_domain.begin(); it != l_domain.end();)
                {
                    // number literals in the with clause are stored as strings (because they may be
                    // arbitrarily long), so convert the stmtnum (actually a prog_line) to string.
                    if(std::to_string(it->getStmtNum()) == num)
                        ++it;
                    else
                        it = l_domain.erase(it);
                }

                tbl->putDomain(l_decl, l_domain);
            }
            else if(right->isDeclaration())
            {
                auto r_decl = right->declaration();
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
                    join_pairs.emplace(entry, Entry(r_decl, entry.getStmtNum()));

                tbl->addJoin(Join(l_decl, r_decl, std::move(join_pairs)));
            }
            else
            {
                assert(right->isAttrRef());
                auto r_ref = right->attrRef();
                auto r_decl = r_ref.decl;
                auto r_domain = tbl->getDomain(r_decl);
                tbl->addSelectDecl(r_decl);

                // the right side is an attrref. we cannot directly perform joins on
                // the extracted attrref, because the domain of the decl must still
                // contain its "original values". eg. for a `call`, it must still be
                // stmt numbers, and not strings if we did `c.procName`.

                // since we know the left side is always a decl, and furthermore it's
                // always a prog_line, we can iterate the right side only and keep this O(n).

                std::unordered_set<std::pair<Entry, Entry>> join_pairs {};
                for(auto it = r_domain.begin(); it != r_domain.end(); ++it)
                {
                    auto r_entry = Table::extractAttr(*it, r_ref, pkb);
                    if(r_ref.attr_name == AttrName::kValue)
                    {
                        join_pairs.emplace(Entry(l_decl, std::stoll(r_entry.getVal())), *it);
                    }
                    else
                    {
                        assert(r_ref.attr_name == AttrName::kStmtNum);
                        join_pairs.emplace(Entry(l_decl, r_entry.getStmtNum()), *it);
                    }
                }

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
                auto r_ref = right->attrRef();
                auto r_decl = r_ref.decl;
                auto r_domain = tbl->getDomain(r_decl);
                tbl->addSelectDecl(r_decl);

                auto l_attr = l_ref.attr_name;
                auto r_attr = r_ref.attr_name;

                std::unordered_set<std::pair<Entry, Entry>> join_pairs {};

                // always iterate the smaller domain
                if(r_domain.size() < l_domain.size())
                {
                    std::swap(l_domain, r_domain);
                    std::swap(l_attr, r_attr);
                    std::swap(l_decl, r_decl);
                    std::swap(l_ref, r_ref);
                }

                // same as the above with rhs=attrref, but this time we don't have the guarantee that
                // the left side is always a prog_line. this explodes the checking space.
                // instead of doing an O(n^2) check, special case all possible things.
                if((l_attr == AttrName::kValue || l_attr == AttrName::kStmtNum) &&
                    (r_attr == AttrName::kValue || r_attr == AttrName::kStmtNum))
                {
                    if(l_attr == r_attr)
                    {
                        if(l_attr == AttrName::kValue)
                        {
                            for(const auto& lent : l_domain)
                                join_pairs.emplace(lent, Entry(r_decl, lent.getVal()));
                        }
                        else
                        {
                            for(const auto& lent : l_domain)
                                join_pairs.emplace(lent, Entry(r_decl, lent.getStmtNum()));
                        }
                    }
                    else
                    {
                        if(l_attr == AttrName::kValue && r_attr == AttrName::kStmtNum)
                        {
                            for(const auto& lent : l_domain)
                                join_pairs.emplace(lent, Entry(r_decl, std::stoll(lent.getVal())));
                        }
                        else
                        {
                            for(const auto& lent : l_domain)
                                join_pairs.emplace(lent, Entry(r_decl, std::to_string(lent.getStmtNum())));
                        }
                    }
                }
                else
                {
                    for(const auto& lent : l_domain)
                    {
                        auto lattrval = Table::extractAttr(lent, l_ref, pkb);
                        if(r_attr == AttrName::kProcName)
                        {
                            if(r_decl->design_ent == DESIGN_ENT::PROCEDURE)
                            {
                                join_pairs.emplace(lent, Entry(r_decl, lattrval.getVal()));
                            }
                            else if(r_decl->design_ent == DESIGN_ENT::CALL)
                            {
                                auto& proc = pkb->getProcedureNamed(lattrval.getVal());
                                for(auto& i : proc.getCallStmts())
                                    join_pairs.emplace(lent, Entry(r_decl, i));
                            }
                            else
                            {
                                throw PqlException("pql::eval", "unreachable");
                            }
                        }
                        else if(r_attr == AttrName::kVarName)
                        {
                            if(r_decl->design_ent == DESIGN_ENT::VARIABLE)
                            {
                                join_pairs.emplace(lent, Entry(r_decl, lattrval.getVal()));
                            }
                            else if(r_decl->design_ent == DESIGN_ENT::PRINT)
                            {
                                auto& var = pkb->getVariableNamed(lattrval.getVal());
                                for(auto& i : var.getPrintStmts())
                                    join_pairs.emplace(lent, Entry(r_decl, i));
                            }
                            else if(r_decl->design_ent == DESIGN_ENT::READ)
                            {
                                auto& var = pkb->getVariableNamed(lattrval.getVal());
                                for(auto& i : var.getReadStmts())
                                    join_pairs.emplace(lent, Entry(r_decl, i));
                            }
                            else
                            {
                                throw PqlException("pql::eval", "unreachable");
                            }
                        }
                        else
                        {
                            throw PqlException("pql::eval", "unreachable");
                        }
                    }
                }

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
