// test_pattern.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

struct Runner
{
    Runner(bool should_pass, zst::str_view source, zst::str_view query)
        : m_should_pass(should_pass), m_source(source), m_pkb(nullptr), m_query(query) { }

    Runner(bool should_pass, pkb::ProgramKB* pkb, zst::str_view query)
        : m_should_pass(should_pass), m_source(""), m_pkb(pkb), m_query(query) { }


    std::unordered_set<std::string> run()
    {
        if(!m_pkb)
        {
            auto prog = simple::parser::parseProgram(m_source).unwrap();
            m_pkb = pkb::processProgram(prog).unwrap();
        }

        auto query = pql::parser::parsePQL(m_query);
        auto eval = pql::eval::Evaluator(m_pkb, query);

        auto res = eval.evaluate();
        return std::unordered_set<std::string>(res.begin(), res.end());
    }

    bool m_should_pass;
    zst::str_view m_source;
    pkb::ProgramKB* m_pkb;
    zst::str_view m_query;
};

template <typename... Args>
static std::unordered_set<std::string> make_set(Args&&... args)
{
    auto ret = std::unordered_set<std::string> {};
    (ret.insert(std::to_string(static_cast<Args&&>(args))), ...);

    return ret;
}

#define TEST_OK(source, query, ...)     CHECK(Runner(true, source, query).run() == make_set(__VA_ARGS__))
#define TEST_EMPTY(source, query)       CHECK(Runner(true, source, query).run() == make_set())
#define TEST_ERR(source, query, msg)    CHECK_THROWS_WITH(Runner(false, source, query).run(), Catch::Matchers::Contains(msg))

constexpr const auto test_program = R"(
    procedure Example {
        x = 6;
        y = 18;
        z = 12;

        t1 = x + y + z;
        t2 = x + y + z - z * z;
        t3 = y * (w + x) - (w - y * z) - (w - x) - z;

        t4 = a + b * c - d / e + f * g - h % (i + j - k) * l / ((m - m) * (n + o));

        x = 6 + 1 / 2 - 3;
        y = 69 * 420 - 123;
        z = a + (b + c);

        read q;
    }
)";


TEST_CASE("Select pattern assign(name, _)")
{
    SECTION("positive cases")
    {
        TEST_OK(test_program, R"(assign a; Select a pattern a("x", _))", 1, 8);
        TEST_OK(test_program, R"(assign a; Select a pattern a("y", _))", 2, 9);
        TEST_OK(test_program, R"(assign a; Select a pattern a("z", _))", 3, 10);
    }

    SECTION("empty cases")
    {
        TEST_EMPTY(test_program, "assign a;Select a pattern a(\"q\", _)");
    }
}

TEST_CASE("Select pattern assign(name, _subexpr_)")
{
    // we're running a lot of things, so save time here by only processing once
    auto prog = simple::parser::parseProgram(test_program).unwrap();
    auto pkb = pkb::processProgram(prog).unwrap();

    SECTION("xyz")
    {
        TEST_OK(pkb, R"^(assign a; Select a pattern a("x", _"1"_))^", 8);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("x", _"6"_))^", 1, 8);

        TEST_OK(pkb, R"^(assign a; Select a pattern a("y", _"69"_))^", 9);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("z", _"12"_))^", 3);
    }

    SECTION("t1")
    {
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t1", _"(x)"_))^", 4);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t1", _"(y)"_))^", 4);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t1", _"(z)"_))^", 4);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t1", _"x + y"_))^", 4);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t1", _"x + y + z"_))^", 4);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t1", _"y + z"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t1", _"69"_))^");
    }

    SECTION("t2")
    {
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t2", _"(x)"_))^", 5);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t2", _"(y)"_))^", 5);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t2", _"(z)"_))^", 5);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t2", _"x + y"_))^", 5);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t2", _"z * z"_))^", 5);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t2", _"x + y + z"_))^", 5);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t2", _"y + z"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t2", _"z - z"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t2", _"420"_))^");
    }

    SECTION("t3")
    {
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", _"(x)"_))^", 6);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", _"(y)"_))^", 6);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", _"(z)"_))^", 6);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", _"w + x"_))^", 6);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", _"y * z"_))^", 6);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", _"y * (w + x)"_))^", 6);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", _"w - (y * z)"_))^", 6);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", _"y * (w + x) - (w - y * z)"_))^", 6);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t3", _"w - y"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t3", _"y * w"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t3", _"(w - x) - z"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t3", _"asdf"_))^");
    }

    SECTION("t4")
    {
        // for reference:
        // ((((a + (b * c)) - (d / e)) + (f * g)) - (((h % ((i + j) - k)) * l) / ((m - m) * (n + o))))
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"(a)"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"(f)"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"(o)"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"f * g"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"d / e"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"b * c"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"i + j"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"m - m"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"i + j - k"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"a + b * c"_))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", _"h % (i + j - k)"_))^", 7);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", _"a + b"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", _"j - k"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", _"h % i"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", _"c - d"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", _"m * n"_))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", _"g - h % i"_))^");
    }
}

TEST_CASE("Select pattern assign(name, fullexpr)")
{
    auto prog = simple::parser::parseProgram(test_program).unwrap();
    auto pkb = pkb::processProgram(prog).unwrap();

    SECTION("xyz")
    {
        TEST_OK(pkb, R"^(assign a; Select a pattern a("x", "6"))^", 1);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("y", "18"))^", 2);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("z", "12"))^", 3);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("x", "3"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("x", "2"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("y", "69"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("z", "a"))^");
    }

    SECTION("t1")
    {
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t1", "x + y + z"))^", 4);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t1", "(x + y) + z"))^", 4);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t1", "x + y"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t1", "y + z"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t1", "x + (y + z)"))^");
    }

    SECTION("t2")
    {
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t2", "x + y + z - z * z"))^", 5);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t2", "((x + y) + z) - (z * z)"))^", 5);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t2", "x + y"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t2", "z * z"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t2", "x + y + z"))^");
    }

    SECTION("t3")
    {
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", "y * (w + x) - (w - y * z) - (w - x) - z"))^", 6);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t3", "((y * (w + x)) - (w - (y * z))) - (w - x) - z"))^", 6);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t3", "w + x"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t3", "y * w"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t3", "w - y * z"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t3", "w - x"))^");
    }

    SECTION("t4")
    {
        // for reference:
        // ((((a + (b * c)) - (d / e)) + (f * g)) - (((h % ((i + j) - k)) * l) / ((m - m) * (n + o))))
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", "a + b * c - d / e + f * g - h % (i + j - k) * l / ((m - m) * (n + o))"))^", 7);
        TEST_OK(pkb, R"^(assign a; Select a pattern a("t4", "((((a + (b * c)) - (d / e)) + (f * g))
            - (((h % ((i + j) - k)) * l) / ((m - m) * (n + o))))"))^", 7);

        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", "d / e"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", "i + j"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", "h % i"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", "c - d"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", "m * n"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", "b * c"))^");
        TEST_EMPTY(pkb, R"^(assign a; Select a pattern a("t4", "g - h % i"))^");
    }
}

























