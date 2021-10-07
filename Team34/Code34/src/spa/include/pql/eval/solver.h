// solver.h
//
// table.h Solver

#pragma once

#include <pql/parser/ast.h>
#include <pql/eval/table.h>

namespace pql::eval::solver
{
    // Intermediate Row for IntTable
    class IntRow
    {
    private:
        // mapping of declaration and the index it belongs to
        std::unordered_map<const ast::Declaration*, table::Entry> m_columns;

    public:
        IntRow(const std::unordered_map<const ast::Declaration*, table::Entry>& columns);
        IntRow();
        // merge this row with a new column and return a new copy
        IntRow addColumn(const ast::Declaration* decl, const table::Entry& entry) const;
        bool canMerge(const IntRow& other) const;
        IntRow mergeRow(const IntRow& other) const;
        std::unordered_set<const ast::Declaration*> getHeaders() const;
        table::Entry getVal(const ast::Declaration* decl) const;
        bool contains(const ast::Declaration* decl) const;
        // check columns in the row exist in one of the allowed joins
        bool isAllowed(const table::Join& join) const;
        std::string toString() const;
    };

    // Intermediate Table for solver
    class IntTable
    {
    private:
        std::vector<IntRow> m_rows;
        // header for rows
        std::unordered_set<const ast::Declaration*> m_headers;

    public:
        IntTable(const std::vector<IntRow>& rows, const std::unordered_set<const ast::Declaration*>& headers);
        IntTable();
        bool contains(const ast::Declaration* declaration);
        // Performs cross product on the rows and returns a new IntTable (O(N^2))
        IntTable merge(const IntTable& other);
        // Performs cross product on the Domain and return a new IntTable (O(N^2))
        IntTable mergeColumn(const ast::Declaration* decl, const table::Domain& domain) const;
        std::unordered_set<const ast::Declaration*> getHeaders() const;
        std::vector<IntRow> getRows() const;
        void filterRows(const table::Join& join);
        bool empty();
        std::string toString() const;
    };

    class Solver
    {
    private:
        std::unordered_map<ast::Declaration*, table::Domain> m_domains;
        std::vector<table::Join> m_joins;
        std::unordered_set<ast::Declaration*> m_return_decls;
        std::unordered_set<ast::Declaration*> m_select_decls;


    public:
        Solver(std::vector<table::Join> joins, std::unordered_map<ast::Declaration*, table::Domain> domains,
            std::unordered_set<ast::Declaration*> return_decls, std::unordered_set<ast::Declaration*> select_decls);
    };

}
