// runner.h

#pragma once

#include <type_traits>

#include "exceptions.h"
#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

struct Runner
{
    inline Runner(zst::str_view source, zst::str_view query) : m_source(source), m_pkb(nullptr), m_query(query) { }

    inline Runner(pkb::ProgramKB* pkb, zst::str_view query) : m_source(""), m_pkb(pkb), m_query(query) { }

    inline std::unordered_set<std::string> run(bool try_catch = false)
    {
        if(!m_pkb)
        {
            auto prog = simple::parser::parseProgram(m_source).unwrap();
            m_pkb = pkb::processProgram(prog).unwrap();
        }

        auto query = pql::parser::parsePQL(m_query);
        auto eval = pql::eval::Evaluator(m_pkb, query);

        std::list<std::string> result {};
        if(try_catch)
        {
            try
            {
                result = eval.evaluate();
            }
            catch(const util::Exception& e)
            {
                return {};
            }
        }
        else
        {
            result = eval.evaluate();
        }

        return std::unordered_set<std::string>(result.begin(), result.end());
    }

    zst::str_view m_source;
    pkb::ProgramKB* m_pkb;
    zst::str_view m_query;
};

template <typename T>
static std::string to_string(T x)
{
    if constexpr(std::is_same_v<T, const char*>)
        return std::string(x);
    else
        return std::to_string(x);
}

template <typename... Args>
static std::unordered_set<std::string> make_set(Args&&... args)
{
    auto ret = std::unordered_set<std::string> {};
    (ret.insert(to_string(static_cast<Args&&>(args))), ...);

    return ret;
}

#define TEST_OK(source, query, ...) CHECK(Runner(source, query).run() == make_set(__VA_ARGS__))
#define TEST_EMPTY(source, query) CHECK(Runner(source, query).run(true) == make_set())
#define TEST_ERR(source, query, msg) CHECK_THROWS_WITH(Runner(source, query).run(), Catch::Matchers::Contains(msg))
