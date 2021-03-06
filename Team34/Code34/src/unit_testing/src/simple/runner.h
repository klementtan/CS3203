// test.h

#pragma once

#include <zpr.h>
#include "util.h"

struct Test
{
    bool whole_prog = false;
    bool should_pass = false;

    zst::str_view input {};
    zst::str_view expected {};

    bool run() const;

    static Test expr(bool pass, zst::str_view expr, zst::str_view exp)
    {
        Test ret {};
        ret.whole_prog = false;
        ret.should_pass = pass;
        ret.input = expr;
        ret.expected = exp;
        return ret;
    }

    static Test program(bool pass, zst::str_view prog, zst::str_view exp)
    {
        Test ret {};
        ret.whole_prog = true;
        ret.should_pass = pass;
        ret.input = prog;
        ret.expected = exp;
        return ret;
    }
};

#define TEST_EXPR_OK(input, expected) CHECK(Test::expr(true, input, expected).run())
#define TEST_EXPR_ERR(input, expected) CHECK(Test::expr(false, input, expected).run())

#define TEST_PROG_OK(input, expected) CHECK(Test::program(true, input, expected).run())
#define TEST_PROG_ERR(input, expected) CHECK(Test::program(false, input, expected).run())
