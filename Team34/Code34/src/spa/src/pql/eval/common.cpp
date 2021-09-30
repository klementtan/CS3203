// common.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = util::PqlException;

    bool is_concrete(const ast::EntRef* ref)
    {
        return ref->isName();
    }

    bool is_concrete(const ast::StmtRef* ref)
    {
        return ref->isStatementId();
    }

    std::string get_concrete_value(const ast::EntRef* ref)
    {
        return ref->name();
    }

    pkb::StatementNum get_concrete_value(const ast::StmtRef* ref)
    {
        return ref->id();
    }





    template <typename Entity, typename RelationParam, typename RefType, bool SetsAreConstRef>
    void RelationAbstractor<Entity, RelationParam, RefType, SetsAreConstRef>::evaluate(const pkb::ProgramKB* pkb,
        table::Table* table, const ast::RelCond* rel, const RefType* leftRef, const RefType* rightRef) const
    {
        if(leftRef->isDeclaration())
        {
            table->addSelectDecl(leftRef->declaration());
            if(auto lde = this->leftDeclEntity; lde && leftRef->declaration()->design_ent != *lde)
            {
                throw PqlException("pql::eval", "entity for first argument of '{}' must be a {}", this->relationName,
                    ast::INV_DESIGN_ENT_MAP.at(*lde));
            }
        }

        if(rightRef->isDeclaration())
        {
            table->addSelectDecl(rightRef->declaration());
            if(auto rde = this->rightDeclEntity; rde && rightRef->declaration()->design_ent != *rde)
            {
                throw PqlException("pql::eval", "entity for second argument of '{}' must be a {}", this->relationName,
                    ast::INV_DESIGN_ENT_MAP.at(*rde));
            }
        }

        auto relation_holds = this->relationHolds;
        auto get_all_related = this->getAllRelated;

        // we can also deduplicate the Rel(X, Y) vs Rel(Y, X) code. in the 3x3 matrix of
        // permutations, this lets us eliminate 3 identical branches, which is a good.
        if((leftRef->isDeclaration() && is_concrete(rightRef)) || (leftRef->isWildcard() && is_concrete(rightRef)) ||
            (leftRef->isWildcard() && rightRef->isDeclaration()))
        {
            relation_holds = this->inverseRelationHolds;
            get_all_related = this->getAllInverselyRelated;

            std::swap(leftRef, rightRef);
        }

        // TODO: see if there's a way to further abstract this to work on both StmtRef and EntRef.
        // it's like 60% abstracted, but there's a bunch of hardcoded stuff for now.
        if(is_concrete(leftRef) && is_concrete(rightRef))
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, EntRef)", this->relationName);
            auto& left_ = (pkb->*getEntity)(get_concrete_value(leftRef));
            auto& right_ = (pkb->*getEntity)(get_concrete_value(rightRef));

            if(!relation_holds(left_, right_))
                throw PqlException("pql::eval", "{} always evaluates to false", rel->toString());
        }
        else if(is_concrete(leftRef) && rightRef->isDeclaration())
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, Decl)", this->relationName);
            auto& left_ = (pkb->*getEntity)(get_concrete_value(leftRef));

            auto domain = table->getDomain(rightRef->declaration());
            for(auto it = domain.begin(); it != domain.end();)
            {
                auto& right_ = (pkb->*getEntity)(((*it).*getEntryValue)());
                if(!relation_holds(left_, right_))
                    it = domain.erase(it);
                else
                    ++it;
            }

            table->upsertDomains(rightRef->declaration(), domain);
        }
        else if(leftRef->isDeclaration() && rightRef->isWildcard())
        {
            util::logfmt("pql::eval", "Processing {}(Decl, _)", this->relationName);
            auto domain = table->getDomain(leftRef->declaration());
            for(auto it = domain.begin(); it != domain.end();)
            {
                auto& left = (pkb->*getEntity)(((*it).*getEntryValue)());
                if(get_all_related(left).empty())
                    it = domain.erase(it);
                else
                    ++it;
            }
            table->upsertDomains(leftRef->declaration(), domain);
        }
        else if(leftRef->isDeclaration() && rightRef->isDeclaration())
        {
            util::logfmt("pql::eval", "Processing {}(Decl, Decl)", this->relationName);

            auto left_decl = leftRef->declaration();
            auto right_decl = rightRef->declaration();

            auto left_domain = table->getDomain(left_decl);
            auto new_right_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> join_pairs;

            for(auto it = left_domain.begin(); it != left_domain.end();)
            {
                auto& left_ = (pkb->*getEntity)(((*it).*getEntryValue)());
                decltype(auto) all_related = get_all_related(left_);
                if(all_related.empty())
                {
                    it = left_domain.erase(it);
                    continue;
                }

                auto left_entry = table::Entry(left_decl, ((*it).*getEntryValue)());
                for(const auto& right_value : all_related)
                {
                    auto right_entry = table::Entry(right_decl, right_value);
                    util::logfmt("pql::eval", "{} adds Join({}, {})", rel->toString(), left_entry.toString(),
                        right_entry.toString());

                    join_pairs.insert({ left_entry, right_entry });
                    new_right_domain.insert(right_entry);
                }
                ++it;
            }

            table->upsertDomains(left_decl, left_domain);
            table->upsertDomains(
                right_decl, table::entry_set_intersect(new_right_domain, table->getDomain(right_decl)));

            table->addJoin(table::Join(left_decl, right_decl, join_pairs));
        }
        else if(is_concrete(leftRef) && rightRef->isWildcard())
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, _)", this->relationName);
            auto& left_ = (pkb->*getEntity)(get_concrete_value(leftRef));
            if(get_all_related(left_).empty())
                throw PqlException("pql::eval", "{} always evaluates to false", rel->toString());
        }
        else if(leftRef->isWildcard() && rightRef->isWildcard())
        {
            util::logfmt("pql::eval", "Processing {}(_, _)", this->relationName);
            if(!(pkb->*relationExists)())
                throw PqlException("pql::eval", "{} always evaluates to false", rel->toString());
        }
        else
        {
            throw PqlException("pql::eval", "unreachable: invalid combination of argument types");
        }
    }

    template struct RelationAbstractor<pkb::Statement, pkb::StatementNum, ast::StmtRef, false>;
    template struct RelationAbstractor<pkb::Statement, pkb::StatementNum, ast::StmtRef, true>;
    template struct RelationAbstractor<pkb::Procedure, std::string, ast::EntRef, true>;
}
