// with.cpp

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

    static void handle_attrref_const(
        const WithCondRef* left, const WithCondRef* right, const pkb::ProgramKB* pkb, Table* tbl)
    {
        auto l_ref = left->attrRef();
        auto l_decl = l_ref.decl;
        auto l_domain = tbl->getDomain(l_ref.decl);

        tbl->addSelectDecl(l_decl);

        for(auto it = l_domain.begin(); it != l_domain.end();)
        {
            auto attr = Table::extractAttr(*it, l_ref, pkb);

            bool equals = false;
            if(l_ref.attr_name == AttrName::kValue)
            {
                spa_assert(right->isNumber());
                equals = right->stringOrNumber() == attr.getVal();
            }
            else if(l_ref.attr_name == AttrName::kStmtNum)
            {
                // TODO: consider changing this to convert the string to a number, since
                // (i think) that is potentially faster.
                spa_assert(right->isNumber());
                equals = right->stringOrNumber() == std::to_string(attr.getStmtNum());
            }
            else
            {
                spa_assert(l_ref.attr_name == AttrName::kProcName || l_ref.attr_name == AttrName::kVarName);
                equals = right->stringOrNumber() == attr.getVal();
            }

            if(!equals)
                it = l_domain.erase(it);
            else
                ++it;
        }

        tbl->putDomain(l_decl, std::move(l_domain));
    }

    static void handle_ref_ref_consts(Domain l_domain, Domain r_domain, Declaration* l_decl, Declaration* r_decl,
        ast::AttrName l_attr, ast::AttrName r_attr, const pkb::ProgramKB* pkb, Table* tbl)
    {
        std::unordered_set<std::pair<Entry, Entry>> join_pairs {};
        Domain new_r_domain {};

        for(auto it = l_domain.begin(); it != l_domain.end();)
        {
            auto& e1 = *it;
            Entry e2 {};
            if(l_attr == r_attr)
            {
                e2 = (r_attr == AttrName::kValue) ? Entry(r_decl, e1.getVal()) : Entry(r_decl, e1.getStmtNum());
            }
            else
            {
                e2 = (r_attr == AttrName::kValue) ? Entry(r_decl, std::to_string(e1.getStmtNum())) :
                                                    Entry(r_decl, std::stoll(e1.getVal()));
            }


            if(r_domain.count(e2) == 0)
            {
                it = l_domain.erase(it);
            }
            else
            {
                join_pairs.emplace(e1, e2);
                new_r_domain.emplace(std::move(e2));
                ++it;
            }
        }

        tbl->putDomain(l_decl, std::move(l_domain));
        tbl->putDomain(r_decl, std::move(new_r_domain));
        tbl->addJoin(Join(l_decl, r_decl, std::move(join_pairs)));
    }

    // return true if there is some valid mapping of `lhs = rhs`. false if there isn't (and so
    // prune this particular lhs from the domain of lhs)
    using EntryPairSet = std::unordered_set<std::pair<Entry, Entry>>;
    static inline bool handle_ref_right_procname(const Domain& old_r_domain, Domain& new_r_domain,
        EntryPairSet& join_pairs, const Entry& lent, const Entry& lattrval, Declaration* r_decl,
        const pkb::ProgramKB* pkb, Table* tbl)
    {
        if(r_decl->design_ent == DESIGN_ENT::PROCEDURE)
        {
            auto e = Entry(r_decl, lattrval.getVal());
            if(old_r_domain.count(e) == 0)
                return false;

            join_pairs.emplace(lent, e);
            new_r_domain.emplace(std::move(e));
            return true;
        }
        else if(r_decl->design_ent == DESIGN_ENT::CALL)
        {
            auto proc = pkb->maybeGetProcedureNamed(lattrval.getVal());
            if(proc == nullptr)
                return false;

            bool has_valid_rhs = false;
            for(auto& i : proc->getCallStmts())
            {
                auto e = Entry(r_decl, i);
                if(old_r_domain.count(e) == 0)
                    continue;

                join_pairs.emplace(lent, e);
                new_r_domain.emplace(std::move(e));
                has_valid_rhs = true;
            }

            return has_valid_rhs;
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }
    }

    static inline bool handle_ref_right_varname(const Domain& old_r_domain, Domain& new_r_domain,
        EntryPairSet& join_pairs, const Entry& lent, const Entry& lattrval, Declaration* r_decl,
        const pkb::ProgramKB* pkb, Table* tbl)
    {
        if(r_decl->design_ent == DESIGN_ENT::VARIABLE)
        {
            auto e = Entry(r_decl, lattrval.getVal());
            if(old_r_domain.count(e) == 0)
                return false;

            join_pairs.emplace(lent, e);
            new_r_domain.emplace(std::move(e));
            return true;
        }
        else if(r_decl->design_ent == DESIGN_ENT::PRINT || r_decl->design_ent == DESIGN_ENT::READ)
        {
            auto var = pkb->maybeGetVariableNamed(lattrval.getVal());
            if(var == nullptr)
                return false;

            auto& rhses = r_decl->design_ent == DESIGN_ENT::PRINT ? var->getPrintStmts() : var->getReadStmts();

            bool has_valid_rhs = false;
            for(auto& i : rhses)
            {
                auto e = Entry(r_decl, i);
                if(old_r_domain.count(e) == 0)
                    continue;

                join_pairs.emplace(lent, e);
                new_r_domain.emplace(std::move(e));
                has_valid_rhs = true;
            }
            return has_valid_rhs;
        }
        else
        {
            throw PqlException("pql::eval", "unreachable");
        }

        return true;
    }



    static void handle_attrref_attrref(
        const WithCondRef* left, const WithCondRef* right, const pkb::ProgramKB* pkb, Table* tbl)
    {
        auto l_ref = left->attrRef();
        auto l_decl = l_ref.decl;
        auto l_domain = tbl->getDomain(l_ref.decl);

        auto r_ref = right->attrRef();
        auto r_decl = r_ref.decl;
        auto r_domain = tbl->getDomain(r_decl);

        tbl->addSelectDecl(l_decl);
        tbl->addSelectDecl(r_decl);

        auto l_attr = l_ref.attr_name;
        auto r_attr = r_ref.attr_name;

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
            handle_ref_ref_consts(std::move(l_domain), std::move(r_domain), l_decl, r_decl, l_attr, r_attr, pkb, tbl);
        }
        else
        {
            Domain new_r_domain {};
            std::unordered_set<std::pair<Entry, Entry>> join_pairs {};
            for(auto it = l_domain.begin(); it != l_domain.end();)
            {
                bool should_keep = false;

                auto lattrval = Table::extractAttr(*it, l_ref, pkb);
                if(r_attr == AttrName::kProcName)
                {
                    should_keep =
                        handle_ref_right_procname(r_domain, new_r_domain, join_pairs, *it, lattrval, r_decl, pkb, tbl);
                }
                else if(r_attr == AttrName::kVarName)
                {
                    should_keep =
                        handle_ref_right_varname(r_domain, new_r_domain, join_pairs, *it, lattrval, r_decl, pkb, tbl);
                }
                else
                {
                    throw PqlException("pql::eval", "unreachable");
                }

                if(should_keep)
                    ++it;
                else
                    it = l_domain.erase(it);
            }

            tbl->putDomain(l_decl, std::move(l_domain));
            tbl->putDomain(r_decl, std::move(new_r_domain));
            tbl->addJoin(Join(l_decl, r_decl, std::move(join_pairs)));
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

        spa_assert(!left->isNumber() && !left->isString());

        // in actual fact, we should not have declarations here, i think.
        if(left->isDeclaration())
            throw PqlException("pql::eval", "declaration in 'with' should have been desugared into '.stmt#'!");

        if(!left->isAttrRef())
            throw PqlException("pql::eval", "unreachable");



        if(right->isNumber() || right->isString())
            handle_attrref_const(left, right, pkb, tbl);

        else if(right->isAttrRef())
            handle_attrref_attrref(left, right, pkb, tbl);

        else
            throw PqlException("pql::eval", "unreachable");
    }
}
