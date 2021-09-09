// test.h

#pragma once

#include <zpr.h>
#include <zst.h>

struct Test
{
    bool whole_prog = false;
    bool should_pass = false;

    zst::str_view input {};
    zst::str_view expected {};

    bool run() const;

    static constexpr Test expr(bool pass, zst::str_view expr, zst::str_view exp)
    {
        Test ret {};
        ret.whole_prog = false;
        ret.should_pass = pass;
        ret.input = expr;
        ret.expected = exp;
        return ret;
    }

    static constexpr Test program(bool pass, zst::str_view prog, zst::str_view exp)
    {
        Test ret {};
        ret.whole_prog = true;
        ret.should_pass = pass;
        ret.input = prog;
        ret.expected = exp;
        return ret;
    }
};

#define TEST_EXPR_OK(input, expected) REQUIRE(Test::expr(true, input, expected).run())
#define TEST_EXPR_ERR(input, expected) REQUIRE(Test::expr(false, input, expected).run())

#define TEST_PROG_OK(input, expected) REQUIRE(Test::program(true, input, expected).run())
#define TEST_PROG_ERR(input, expected) REQUIRE(Test::program(false, input, expected).run())
