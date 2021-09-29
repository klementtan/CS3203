// follows.cpp

#include <cassert>
#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = util::PqlException;
    using ProcedureNameSet = std::unordered_set<std::string>;

    // this... is a strategy to reduce code duplication.
    template <typename Entity, typename RelationParam>
    struct StarAbstractor
    {
        // Calls[*](A, B) <=> A.relationHolds(B) <=> B.inverseRelationHolds(A)
        bool (Entity::*relationHolds)(const RelationParam&) const;
        bool (Entity::*inverseRelationHolds)(const RelationParam&) const;

        // Calls[*](A, _) <=> A.getAllRelated()
        // Calls[*](_, B) <=> B.getAllInverselyRelated()
        const std::unordered_set<RelationParam>& (pkb::Procedure::*getAllRelated)() const;
        const std::unordered_set<RelationParam>& (pkb::Procedure::*getAllInverselyRelated)() const;
    };

    // as an experiment, I'm making this as general as possible.
    template <typename RefTy, typename SA_Entity, typename SA_RelationParam>
    static void evaluate_relation(const char* rel_name, const ast::RelCond* rel,
        StarAbstractor<SA_Entity, SA_RelationParam> fns, table::Table* table, const pkb::ProgramKB* pkb,
        const RefTy* left, ast::DESIGN_ENT left_decl_ent, const RefTy* right, ast::DESIGN_ENT right_decl_ent)
    {
        if(left->isDeclaration())
        {
            table->addSelectDecl(left->declaration());
            if(left->declaration()->design_ent != left_decl_ent)
            {
                throw PqlException("pql::eval", "entity for first argument of '{}' must be a {}", rel_name,
                    ast::INV_DESIGN_ENT_MAP.at(left_decl_ent));
            }
        }

        if(right->isDeclaration())
        {
            table->addSelectDecl(right->declaration());
            if(right->declaration()->design_ent != right_decl_ent)
            {
                throw PqlException("pql::eval", "entity for second argument of '{}' must be a {}", rel_name,
                    ast::INV_DESIGN_ENT_MAP.at(right_decl_ent));
            }
        }

        // to prevent the syntax from becoming atrocious
        // eg. ((pkb->getProcedureNamed(left->name())).*(fns.relationHolds))(right->name())
        auto relationHolds = fns.relationHolds;
        auto invRelationHolds = fns.inverseRelationHolds;

        auto getAllRelated = fns.getAllRelated;
        auto getAllInverselyRelated = fns.getAllInverselyRelated;

        // we can also deduplicate the Rel(X, Y) vs Rel(Y, X) code. in the 3x3 matrix of
        // permutations, this lets us eliminate 3 identical branches, which is a good.
        if((left->isDeclaration() && right->isName()) || (left->isWildcard() && right->isName()) ||
            (left->isWildcard() && right->isDeclaration()))
        {
            std::swap(relationHolds, invRelationHolds);
            std::swap(getAllRelated, getAllInverselyRelated);
            std::swap(left_decl_ent, right_decl_ent);
            std::swap(left, right);
        }

        // TODO: see if there's a way to further abstract this to work on both StmtRef and EntRef.
        // it's like 60% abstracted, but there's a bunch of hardcoded stuff for now.
        if(left->isName() && right->isName())
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, EntRef)", rel_name);
            auto& left_ = pkb->getProcedureNamed(left->name());
            if(!(left_.*relationHolds)(right->name()))
                throw PqlException("pql::eval", "{} always evalutes to false", rel->toString());
        }
        else if(left->isName() && right->isDeclaration())
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, Decl)", rel_name);
            auto& left_ = pkb->getProcedureNamed(left->name());

            auto domain = table->getDomain(right->declaration());
            for(auto it = domain.begin(); it != domain.end();)
            {
                if(!(left_.*relationHolds)(it->getVal()))
                    it = domain.erase(it);
                else
                    ++it;
            }

            table->upsertDomains(right->declaration(), domain);
        }
        else if(left->isDeclaration() && right->isWildcard())
        {
            util::logfmt("pql::eval", "Processing {}(Decl, _)", rel_name);
            auto domain = table->getDomain(left->declaration());
            for(auto it = domain.begin(); it != domain.end();)
            {
                auto& proc = pkb->getProcedureNamed(it->getVal());
                if((proc.*getAllRelated)().empty())
                    it = domain.erase(it);
                else
                    ++it;
            }
            table->upsertDomains(left->declaration(), domain);
        }
        else if(left->isDeclaration() && right->isDeclaration())
        {
            util::logfmt("pql::eval", "Processing {}(Decl, Decl)", rel_name);

            auto left_decl = left->declaration();
            auto right_decl = right->declaration();

            auto left_domain = table->getDomain(left_decl);
            auto new_right_domain = table::Domain {};
            std::unordered_set<std::pair<table::Entry, table::Entry>> join_pairs;

            for(auto it = left_domain.begin(); it != left_domain.end();)
            {
                auto& all_related = (pkb->getProcedureNamed(it->getVal()).*getAllRelated)();
                if(all_related.empty())
                {
                    it = left_domain.erase(it);
                    continue;
                }

                auto left_entry = table::Entry(left_decl, it->getVal());
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
        else if(left->isName() && right->isWildcard())
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, _)", rel_name);
            auto& left_ = pkb->getProcedureNamed(left->name());
            if((left_.*getAllRelated)().empty())
                throw PqlException("pql::eval", "{} always evalutes to false", rel->toString());
        }
        else if(left->isWildcard() && right->isWildcard())
        {
            util::logfmt("pql::eval", "Processing {}(_, _)", rel_name);
            if(!pkb->callsRelationExists())
                throw PqlException("pql::eval", "{} always evalutes to false", rel->toString());
        }
        else
        {
            throw PqlException("pql::eval", "unreachable: invalid combination of argument types");
        }
    }

    void Evaluator::handleCalls(const ast::Calls* rel)
    {
        assert(rel);

        StarAbstractor<pkb::Procedure, std::string> abs {};
        abs.relationHolds = &pkb::Procedure::callsProcedure;
        abs.inverseRelationHolds = &pkb::Procedure::isCalledByProcedure;
        abs.getAllRelated = &pkb::Procedure::getAllCalledProcedures;
        abs.getAllInverselyRelated = &pkb::Procedure::getAllCallers;

        evaluate_relation("Calls", rel, std::move(abs), &m_table, m_pkb, &rel->caller, ast::DESIGN_ENT::PROCEDURE,
            &rel->proc, ast::DESIGN_ENT::PROCEDURE);
    }

    void Evaluator::handleCallsT(const ast::CallsT* rel)
    {
        assert(rel);

        StarAbstractor<pkb::Procedure, std::string> abs {};
        abs.relationHolds = &pkb::Procedure::callsProcedureTransitively;
        abs.inverseRelationHolds = &pkb::Procedure::isTransitivelyCalledByProcedure;
        abs.getAllRelated = &pkb::Procedure::getAllTransitivelyCalledProcedures;
        abs.getAllInverselyRelated = &pkb::Procedure::getAllTransitiveCallers;

        evaluate_relation("Calls*", rel, std::move(abs), &m_table, m_pkb, &rel->caller, ast::DESIGN_ENT::PROCEDURE,
            &rel->proc, ast::DESIGN_ENT::PROCEDURE);
    }
}
