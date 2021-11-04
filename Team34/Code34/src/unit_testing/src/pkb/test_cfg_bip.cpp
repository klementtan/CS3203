// cfg_bip.cpp

#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "design_extractor.h"
#include "simple/parser.h"
#include "pkb.h"
#include "util.h"
#include <zpr.h>
using namespace simple::parser;
using namespace pkb;

constexpr const auto sample_source_A = R"(
procedure Bill {
      x = 5;
      call Mary;
      y = x + 6;
      call John;
      z = x * y + 2; }

procedure Mary {
      y = x * 3;
      call John;
      z = x + y; }

procedure John {
      if (i > 0) then {
              x = x + z; }
      else {
              y = x * y; } }
)";

constexpr const auto sample_source_B = R"(
procedure B {
        call C;
        call C;
        call C; }

procedure C {
        d = a;
        a = b;
        b = c;
        c = d; }
)";

constexpr const auto sample_source_C = R"(
procedure P {
      call Q1;
      while (i > 0) {
              call Q1; }
      if (i > 0) then {
              call Q2; }
        else {
              call Q3; } }

procedure Q1 {
      call Q2;}

procedure Q2 {
      call Q3;}

procedure Q3 {
      a = a;}
)";

static auto kb1 = DesignExtractor(parseProgram(sample_source_A)).run();
static auto cfg1 = kb1 -> getCFG();

static auto kb2 = DesignExtractor(parseProgram(sample_source_B)).run();
static auto cfg2 = kb2 -> getCFG();

static auto kb3 = DesignExtractor(parseProgram(sample_source_C)).run();
static auto cfg3 = kb3 -> getCFG();

TEST_CASE("NextBip and NextBip*")
{
    SECTION("positive test cases: bip")
    {
        CHECK(cfg1->isStatementNextBip(2, 6));
        CHECK(cfg1->isStatementNextBip(9, 10));
        CHECK(cfg1->isStatementNextBip(9, 11));
        CHECK(cfg1->isStatementNextBip(11, 8));
        CHECK(cfg2->isStatementNextBip(1, 4));
        CHECK(cfg2->isStatementNextBip(7, 3));
        CHECK(cfg3->isStatementNextBip(9, 2));

        CHECK(cfg1->isStatementTransitivelyNextBip(2, 6));
        CHECK(cfg1->isStatementTransitivelyNextBip(1, 11));
        CHECK(cfg1->isStatementTransitivelyNextBip(1, 5));
        CHECK(cfg1->isStatementTransitivelyNextBip(4, 5));
        CHECK(cfg1->isStatementTransitivelyNextBip(9, 5));
        CHECK(cfg1->isStatementTransitivelyNextBip(10, 11));
        CHECK(cfg2->isStatementTransitivelyNextBip(3, 4));
        CHECK(cfg2->isStatementTransitivelyNextBip(1, 3));
        CHECK(cfg3->isStatementTransitivelyNextBip(9, 4));


        CHECK(cfg1->getNextStatementsBip(9) == StatementSet { 10, 11 });
        CHECK(cfg2->getNextStatementsBip(7) == StatementSet { 2, 3 });
        CHECK(cfg1->getPreviousStatementsBip(7) == StatementSet { 6 });
    }
    SECTION("negative test cases: bip")
    {
        CHECK(!cfg1->isStatementNextBip(2, 7));
        CHECK(!cfg1->isStatementNextBip(2, 3));
        CHECK(!cfg2->isStatementNextBip(1, 7));
        CHECK(!cfg2->isStatementNextBip(1, 2));
        CHECK(!cfg2->isStatementNextBip(7, 7));
        CHECK(!cfg3->isStatementNextBip(9, 6));
        CHECK(!cfg3->isStatementNextBip(3, 2));

        CHECK(!cfg1->isStatementTransitivelyNextBip(4, 8));
        CHECK(!cfg1->isStatementTransitivelyNextBip(6, 2));
        CHECK(!cfg2->isStatementTransitivelyNextBip(3, 1));

        CHECK(cfg1->getTransitivelyNextStatementsBip(11) == StatementSet { 3, 4, 5, 8, 9, 10, 11 });
        CHECK(cfg1->getTransitivelyPreviousStatementsBip(8) == StatementSet { 1, 2, 6, 7, 9, 10, 11 });
    }
}

TEST_CASE("AffectsBip(a, b)")
{
    SECTION("affects another statement right after another proc call")
    {
        CHECK(cfg1->doesAffectBip(1, 6));
        CHECK(cfg1->doesAffectBip(1, 10));
        CHECK(cfg1->doesAffectBip(1, 11));
        CHECK(cfg2->doesAffectBip(7, 6));
        CHECK(cfg2->doesAffectBip(6, 5));
        CHECK(cfg2->doesAffectBip(5, 4));
    }

    SECTION("affects another statement but prefer one branch")
    {
        CHECK(cfg1->doesAffectBip(1, 3));
        CHECK(cfg1->doesAffectBip(1, 5));
        CHECK(cfg1->doesAffectBip(1, 8));
        CHECK(cfg1->doesAffectBip(3, 5));
        CHECK(cfg1->doesAffectBip(6, 8));
    }

    SECTION("ambiguous return proc call")
    {
        CHECK(cfg1->doesAffectBip(10, 8));
        CHECK(cfg1->doesAffectBip(11, 5));
        CHECK(cfg3->doesAffectBip(9, 9));
    }

    SECTION("negative test cases: not assign statements")
    {
        CHECK(!cfg1->doesAffectBip(1, 2));
        CHECK(!cfg1->doesAffectBip(1, 4));
        CHECK(!cfg1->doesAffectBip(7, 9));
        CHECK(!cfg1->doesAffectBip(7, 11));
    }

    SECTION("negative test cases: variables modified were not used")
    {
        CHECK(!cfg1->doesAffectBip(6, 3));
        CHECK(!cfg1->doesAffectBip(8, 5));
    }
}

TEST_CASE("AffectsBip*(a, b)")
{
    SECTION("should be true when Affects(a, b) holds")
    {
        CHECK(cfg1->doesTransitivelyAffectBip(1, 6));
        CHECK(cfg1->doesTransitivelyAffectBip(1, 10));
        CHECK(cfg1->doesTransitivelyAffectBip(1, 11));
        CHECK(cfg2->doesTransitivelyAffectBip(7, 6));
        CHECK(cfg2->doesTransitivelyAffectBip(6, 5));
        CHECK(cfg2->doesTransitivelyAffectBip(5, 4));
        CHECK(cfg1->doesTransitivelyAffectBip(1, 3));
        CHECK(cfg1->doesTransitivelyAffectBip(1, 5));
        CHECK(cfg1->doesTransitivelyAffectBip(1, 8));
        CHECK(cfg1->doesTransitivelyAffectBip(3, 5));
        CHECK(cfg1->doesTransitivelyAffectBip(6, 8));
        CHECK(cfg1->doesTransitivelyAffectBip(10, 8));
        CHECK(cfg1->doesTransitivelyAffectBip(11, 5));
        CHECK(cfg3->doesTransitivelyAffectBip(9, 9));
    }

    SECTION("there is one intermediary variable")
    {
        CHECK(cfg2->doesTransitivelyAffectBip(6, 4));
        CHECK(cfg2->doesTransitivelyAffectBip(7, 5));
    }

    SECTION("negative test case: impossible based on proc calls")
    {
        CHECK(!cfg2->doesTransitivelyAffectBip(7, 4));
    }
}
