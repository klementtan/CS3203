// common.cpp

#include <algorithm>

#include "exceptions.h"
#include "pql/eval/table.h"
#include "pql/eval/common.h"
#include "pql/eval/evaluator.h"

namespace pql::eval
{
    using PqlException = util::PqlException;

    static inline bool is_concrete(const ast::EntRef* ref)
    {
        return ref->isName();
    }

    static inline bool is_concrete(const ast::StmtRef* ref)
    {
        return ref->isStatementId();
    }

    static inline std::string get_concrete_value(const ast::EntRef* ref)
    {
        return ref->name();
    }

    static inline pkb::StatementNum get_concrete_value(const ast::StmtRef* ref)
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
                    ast::getInverseDesignEntityMap().at(*lde));
            }
            else if(std::is_same_v<RefType, ast::StmtRef> &&
                    ast::getStmtDesignEntities().count(leftRef->declaration()->design_ent) == 0)
            {
                throw PqlException(
                    "pql::eval", "first argument of '{}' must be a statement-like synonym", this->relationName);
            }
        }

        if(rightRef->isDeclaration())
        {
            table->addSelectDecl(rightRef->declaration());
            if(auto rde = this->rightDeclEntity; rde && rightRef->declaration()->design_ent != *rde)
            {
                throw PqlException("pql::eval", "entity for second argument of '{}' must be a {}", this->relationName,
                    ast::getInverseDesignEntityMap().at(*rde));
            }
            else if(std::is_same_v<RefType, ast::StmtRef> &&
                    ast::getStmtDesignEntities().count(rightRef->declaration()->design_ent) == 0)
            {
                throw PqlException(
                    "pql::eval", "second argument of '{}' must be a statement-like synonym", this->relationName);
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


        if(is_concrete(leftRef) && is_concrete(rightRef))
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, EntRef)", this->relationName);
            auto& left_ = (pkb->*getEntity)(get_concrete_value(leftRef));
            auto& right_ = (pkb->*getEntity)(get_concrete_value(rightRef));

            if(!relation_holds(pkb, left_, right_))
                throw PqlException("pql::eval", "{} always evaluates to false", rel->toString());
        }
        else if(is_concrete(leftRef) && rightRef->isDeclaration())
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, Decl)", this->relationName);
            auto& left_ = (pkb->*getEntity)(get_concrete_value(leftRef));

            auto domain = table->getDomain(rightRef->declaration());
            for(auto it = domain.begin(); it != domain.end();)
            {
                auto& right_ = (pkb->*getEntity)(getEntryValue<RelationParam>(*it));
                if(!relation_holds(pkb, left_, right_))
                    it = domain.erase(it);
                else
                    ++it;
            }

            table->putDomain(rightRef->declaration(), domain);
        }
        else if(leftRef->isDeclaration() && rightRef->isWildcard())
        {
            util::logfmt("pql::eval", "Processing {}(Decl, _)", this->relationName);
            auto domain = table->getDomain(leftRef->declaration());
            for(auto it = domain.begin(); it != domain.end();)
            {
                auto& left = (pkb->*getEntity)(getEntryValue<RelationParam>(*it));
                if(get_all_related(pkb, left).empty())
                    it = domain.erase(it);
                else
                    ++it;
            }
            table->putDomain(leftRef->declaration(), domain);
        }
        else if(leftRef->isDeclaration() && rightRef->isDeclaration())
        {
            util::logfmt("pql::eval", "Processing {}(Decl, Decl)", this->relationName);

            auto left_decl = leftRef->declaration();
            auto right_decl = rightRef->declaration();

            evaluateTwoDeclRelations<RelationParam, RelationParam>(pkb, table, rel, left_decl, right_decl,
                [&](const RelationParam& p) -> decltype(auto) { return get_all_related(pkb, (pkb->*getEntity)(p)); });
        }
        else if(is_concrete(leftRef) && rightRef->isWildcard())
        {
            util::logfmt("pql::eval", "Processing {}(EntRef, _)", this->relationName);
            auto& left_ = (pkb->*getEntity)(get_concrete_value(leftRef));
            if(get_all_related(pkb, left_).empty())
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
