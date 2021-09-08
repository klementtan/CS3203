#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include <zst.h>
#include <zpr.h>
#include "simple/parser.h"
#include "pkb.h"
#include "util.h"
using namespace simple::parser;
using namespace pkb;

static void req(bool b)
{
    REQUIRE(b);
}

TEST_CASE("Populate PKB")
{
    SECTION("Cyclic calls")
    {
        constexpr const auto in = R"(
            procedure A {
	            call B;
            }
            procedure B {
	            call C;
            }
            procedure C {
	            call A;
            }
        )";

        auto prog = parseProgram(in);
        auto pkb = processProgram(prog.unwrap());
        std::string expectation = "Cyclic or recursive calls are not allowed";
        req(expectation == pkb.error());
    }

    SECTION("Recursive call")
    {
        constexpr const auto in = R"(
            procedure B {
	            call B;
            }
        )";

        auto prog = parseProgram(in);
        auto pkb = processProgram(prog.unwrap());
        std::string expectation = "Cyclic or recursive calls are not allowed";
        req(expectation == pkb.error());
    }

    SECTION("Recursive calls in disjoint graphs")
    {
        constexpr const auto in = R"(
            procedure A {
	            call B;
            }
            procedure B {
	            call C;
            }
            procedure C {
	            call D;
            }
            procedure G {
	            call G;
            }
            procedure Z {
	            call X;
            }
            procedure X {
	            call V;
            }
            procedure V {
	            a = 1;
            }
            procedure D {
	            a = 1;
            }
        )";

        auto prog = parseProgram(in);
        auto pkb = processProgram(prog.unwrap());
        std::string expectation = "Cyclic or recursive calls are not allowed";
        req(expectation == pkb.error());
    }

    SECTION("Call to non-existent procedure")
    {
        constexpr const auto in = R"(
            procedure A {
	            call B;
            }
            procedure B {
	            call C;
            }
        )";
        auto prog = parseProgram(in);
        auto pkb = processProgram(prog.unwrap());
        std::string expectation = "Procedure 'C' is undefined";
        req(expectation == pkb.error());
    }
}