// test_calls.cpp

#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "design_extractor.h"
#include "simple/parser.h"
#include "pkb.h"
#include "util.h"

using namespace simple::parser;
using namespace pkb;

constexpr const auto src = R"(
    procedure A { call B; }
    procedure B { call C; call D; call E; }
    procedure C { if (1 == 2) then { call D; } else { call E; } }
    procedure D { print X; }
    procedure E { call F; }
    procedure F { call G; call H; }
    procedure G { call X; }
    procedure H { call P; }
    procedure X { print A; }
    procedure P { print P; }
)";

static auto kb = DesignExtractor(parseProgram(src)).run();
static const Procedure& get_proc(const std::unique_ptr<pkb::ProgramKB>& pkb, const std::string& name)
{
    return pkb->getProcedureNamed(name);
}

#define CHECK_CALLS(a, b) do { \
    CHECK(get_proc(kb, a).callsProcedure(b));       \
    CHECK(get_proc(kb, b).isCalledByProcedure(a));  \
} while(0)

#define CHECK_NOT_CALLS(a, b) do { \
    CHECK_FALSE(get_proc(kb, a).callsProcedure(b));       \
    CHECK_FALSE(get_proc(kb, b).isCalledByProcedure(a));  \
} while(0)


TEST_CASE("Calls(a, b)")
{
    CHECK_CALLS("A", "B");
    CHECK_CALLS("B", "C");
    CHECK_CALLS("B", "D");
    CHECK_CALLS("B", "E");
    CHECK_CALLS("C", "D");
    CHECK_CALLS("C", "E");
    CHECK_CALLS("E", "F");
    CHECK_CALLS("F", "G");
    CHECK_CALLS("F", "H");
    CHECK_CALLS("G", "X");
    CHECK_CALLS("H", "P");

    CHECK_NOT_CALLS("A", "C");
    CHECK_NOT_CALLS("A", "D");
    CHECK_NOT_CALLS("A", "E");

    CHECK_NOT_CALLS("A", "X");
    CHECK_NOT_CALLS("A", "P");

    CHECK_NOT_CALLS("D", "A");
    CHECK_NOT_CALLS("D", "E");
    CHECK_NOT_CALLS("D", "X");
    CHECK_NOT_CALLS("X", "A");
    CHECK_NOT_CALLS("P", "P");

    SECTION("testing invalid queries")
    {
        CHECK_THROWS_WITH(get_proc(kb, "10"), Catch::Matchers::Contains("no procedure named '10'"));
    }
}

#define CHECK_CALLS_STAR(a, b) do { \
    CHECK(get_proc(kb, a).callsProcedureTransitively(b));       \
    CHECK(get_proc(kb, b).isTransitivelyCalledByProcedure(a));  \
} while(0)

#define CHECK_NOT_CALLS_STAR(a, b) do { \
    CHECK_FALSE(get_proc(kb, a).callsProcedureTransitively(b));       \
    CHECK_FALSE(get_proc(kb, b).isTransitivelyCalledByProcedure(a));  \
} while(0)

TEST_CASE("Calls*(a, b)")
{
    CHECK_CALLS_STAR("A", "B");
    CHECK_CALLS_STAR("B", "C");
    CHECK_CALLS_STAR("B", "D");
    CHECK_CALLS_STAR("B", "E");
    CHECK_CALLS_STAR("C", "D");
    CHECK_CALLS_STAR("C", "E");
    CHECK_CALLS_STAR("E", "F");
    CHECK_CALLS_STAR("F", "G");
    CHECK_CALLS_STAR("F", "H");
    CHECK_CALLS_STAR("G", "X");
    CHECK_CALLS_STAR("H", "P");

    CHECK_CALLS_STAR("A", "C");
    CHECK_CALLS_STAR("A", "D");
    CHECK_CALLS_STAR("A", "E");
    CHECK_CALLS_STAR("A", "X");
    CHECK_CALLS_STAR("A", "P");

    CHECK_NOT_CALLS_STAR("D", "A");
    CHECK_NOT_CALLS_STAR("D", "E");
    CHECK_NOT_CALLS_STAR("D", "X");
    CHECK_NOT_CALLS_STAR("X", "A");
    CHECK_NOT_CALLS_STAR("P", "P");
}
