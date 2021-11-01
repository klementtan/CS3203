// common.h

#pragma once

#include <list>
#include <memory>

#include "pkb.h"
#include "pql/parser/ast.h"
#include "pql/eval/table.h"
#include "simple/ast.h"

namespace pql::eval
{
    template <typename Entity, typename RelationParam, typename RefType, bool SetsAreConstRef>
    struct RelationAbstractor
    {
        const char* relationName = nullptr;

        // what kind of entity must the declaration be, if the left/right refs are indeed decls.
        // optional; if empty, then this is not enforced.
        std::optional<ast::DESIGN_ENT> leftDeclEntity {};
        std::optional<ast::DESIGN_ENT> rightDeclEntity {};

        // Relation[*](A, B) <=> A.relationHolds(B) <=> B.inverseRelationHolds(A)
        bool (*relationHolds)(const pkb::ProgramKB*, const Entity&, const Entity&) {};
        bool (*inverseRelationHolds)(const pkb::ProgramKB*, const Entity&, const Entity&) {};

        template <typename T>
        using SetWrapper = std::conditional_t<SetsAreConstRef, const std::unordered_set<T>&, std::unordered_set<T>>;

        // Relation[*](A, _) <=> A.getAllRelated()
        // Relation[*](_, B) <=> B.getAllInverselyRelated()
        SetWrapper<RelationParam> (*getAllRelated)(const pkb::ProgramKB*, const Entity&) {};
        SetWrapper<RelationParam> (*getAllInverselyRelated)(const pkb::ProgramKB*, const Entity&) {};

        // getStatementAt, getProcedureNamed, getVariableNamed
        const Entity& (pkb::ProgramKB::*getEntity)(const RelationParam&) const;

        // callsRelationExists, parentRelationExists, etc.
        bool (pkb::ProgramKB::*relationExists)() const;

        void evaluate(const pkb::ProgramKB* pkb, table::Table* table, const ast::RelCond* rel, const RefType* left,
            const RefType* right) const;
    };


    template <typename E>
    inline E getEntryValue(const table::Entry& entry);

    template <>
    inline pkb::StatementNum getEntryValue<pkb::StatementNum>(const table::Entry& entry)
    {
        return entry.getStmtNum();
    }

    template <>
    inline std::string getEntryValue<std::string>(const table::Entry& entry)
    {
        return entry.getVal();
    }

    template <typename LeftRelParam, typename RightRelParam, typename GetAllRelatedToLeftFn>
    void evaluateTwoDeclRelations(const pkb::ProgramKB* pkb, table::Table* table, const ast::RelCond* rel,
        ast::Declaration* left_decl, ast::Declaration* right_decl, GetAllRelatedToLeftFn&& get_all_related)
    {
        auto left_domain = table->getDomain(left_decl);
        auto new_right_domain = table::Domain {};
        std::unordered_set<std::pair<table::Entry, table::Entry>> join_pairs;

        for(auto it = left_domain.begin(); it != left_domain.end();)
        {
            decltype(auto) all_related = get_all_related(getEntryValue<LeftRelParam>(*it));

            if(all_related.empty())
            {
                it = left_domain.erase(it);
                continue;
            }

            auto left_entry = table::Entry(left_decl, getEntryValue<LeftRelParam>(*it));
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

        table->putDomain(left_decl, left_domain);
        table->putDomain(right_decl, table::entry_set_intersect(new_right_domain, table->getDomain(right_decl)));

        table->addJoin(table::Join(left_decl, right_decl, join_pairs));
    }

}
