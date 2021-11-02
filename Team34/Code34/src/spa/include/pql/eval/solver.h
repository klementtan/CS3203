// solver.h
//
// table.h Solver

#pragma once
#include <pql/parser/ast.h>
#include <pql/eval/table.h>

namespace pql::eval::solver
{
    using TableHeaders = std::unordered_set<const ast::Declaration*>;


    // Intermediate Row for IntTable
    class IntRow
    {
    private:
        // the Entry contains a Declaration, so we are able to get the value by a declaration.
        std::vector<table::Entry> m_columns;

    public:
        // explicit IntRow(std::unordered_map<const ast::Declaration*, table::Entry> columns);
        explicit IntRow(std::vector<table::Entry> columns);

        IntRow();
        // merge this row with a new column and return a new copy
        void addColumn(const ast::Declaration* decl, const table::Entry& entry);
        [[nodiscard]] bool canMerge(const IntRow& other, const TableHeaders& other_headers) const;
        // merge other row into current row
        void mergeRow(const IntRow& other, const TableHeaders& other_headers);
        // Remove columns that are not in the allowed headers
        void filterColumns(const TableHeaders& allowed_headers);

        const table::Entry& getVal(const ast::Declaration* decl) const;

        bool contains(const ast::Declaration* decl) const;
        size_t size() const;
        const std::vector<table::Entry>& getColumns() const;
        // check columns in the row exist in one of the allowed joins
        [[nodiscard]] bool isAllowed(const table::Join& join) const;
        [[nodiscard]] std::string toString() const;

        bool operator==(const IntRow& other) const;
    };

    // Intermediate Table for solver
    class IntTable
    {
    private:
        std::vector<IntRow> m_rows;
        // header for rows
        TableHeaders m_headers;

    public:
        IntTable(std::vector<IntRow> rows, const TableHeaders& headers);
        IntTable();
        bool contains(const ast::Declaration* declaration);
        // Performs cross product on the rows
        void merge(const IntTable& other);
        // Performs cross product on the Domain and return a new IntTable (O(N^2))
        void mergeColumn(const ast::Declaration* decl, const table::Domain& domain);
        // Remove columns that are not in the allowed headers
        void filterColumns(const TableHeaders& allowed_headers);
        [[nodiscard]] const TableHeaders& getHeaders() const;
        [[nodiscard]] const std::vector<IntRow>& getRows() const;
        [[nodiscard]] std::vector<IntRow>& getRowsMutable();
        void filterRows(const table::Join& join);
        void dedupRows();
        const IntRow& getRow(size_t i) const;
        IntRow& getRowMutable(size_t i);
        std::vector<IntRow>& getRows();

        [[nodiscard]] bool empty() const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] std::string toString() const;

        [[nodiscard]] size_t numColumns() const;
    };

    // Dependency graph for join conditions
    class DepGraph
    {
    private:
        std::vector<table::Join> m_joins;
        std::unordered_map<const ast::Declaration*, int> m_colouring;
        std::unordered_map<const ast::Declaration*, TableHeaders> m_graph;
        void colour_node(const ast::Declaration* s, int colour);

    public:
        DepGraph(const TableHeaders& decls, std::vector<table::Join> joins);

        [[nodiscard]] std::vector<TableHeaders> getComponents() const;
        [[nodiscard]] std::string toString() const;
    };

    // Solver
    class Solver
    {
    private:
        std::unordered_map<const ast::Declaration*, table::Domain> m_domains;
        std::vector<table::Join> m_joins;
        TableHeaders m_return_decls;
        std::vector<IntTable> m_int_tables;
        std::vector<std::vector<const ast::Declaration*>> m_decl_components;
        DepGraph m_dep_graph;

        std::vector<table::Join> get_joins(const ast::Declaration* decl) const;

        // trim the domain and allowed entries that are associated with decls
        //
        // all allowed entries should be in the domain and all domain should be in the allowed entries
        void trim(const TableHeaders& decls);
        void trim_helper(const ast::Declaration* decls, table::Join& join);
        size_t get_table_index(const ast::Declaration* decl) const;
        // preprocess by joining tables based on the Joins
        void preprocess_int_table();
        bool has_table(const ast::Declaration* decl) const;
        std::vector<std::vector<const ast::Declaration*>> sort_components(
            const std::vector<TableHeaders>& components) const;

    public:
        Solver(const std::vector<table::Join>& joins,
            std::unordered_map<const ast::Declaration*, table::Domain> domains, const TableHeaders& return_decls,
            const TableHeaders& select_decls);
        [[nodiscard]] IntTable getRetTbl();
        [[nodiscard]] bool isValid() const;
        [[nodiscard]] std::string toString() const;
    };

}

template <>
struct std::hash<pql::eval::solver::IntRow>
{
    size_t operator()(const pql::eval::solver::IntRow& r) const
    {
        size_t seed = 0;
        for(const auto& e : r.getColumns())
        {
            util::_hash_combine(seed, util::hash_combine(*e.getDeclaration(), e.getType()));
            if(e.getType() == pql::eval::table::EntryType::kStmt)
                util::_hash_combine(seed, e.getStmtNum());
            else
                util::_hash_combine(seed, e.getVal());
        }

        return seed;
    }
};
