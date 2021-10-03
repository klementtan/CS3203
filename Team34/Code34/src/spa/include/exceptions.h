// exceptions.h

#pragma once

#include <zpr.h>
#include <util.h>

#include <exception>

namespace util
{
    struct Exception : public std::exception
    {
        virtual ~Exception() {};
        virtual const char* what() const noexcept = 0;
    };

    template <typename SubType>
    struct BaseException : Exception
    {
        std::string message;

        template <typename... Args>
        BaseException(const char* who, const char* fmt, const Args&... args)
        {
            util::logfmt(zpr::sprint("{} exception", who).c_str(), fmt, args...);
            this->message = zpr::sprint(fmt, args...);
        }

        const char* what() const noexcept override
        {
            return message.c_str();
        }
    };

    struct PkbException : BaseException<PkbException>
    {
        using BaseException<PkbException>::BaseException;
    };

    struct PqlException : BaseException<PqlException>
    {
        using BaseException<PqlException>::BaseException;
    };

    struct PqlSyntaxException : BaseException<PqlSyntaxException>
    {
        template <typename... Args>
        PqlSyntaxException(const char* fmt, const Args&... args) : BaseException("pql::parser", fmt, args...)
        {
        }
    };

    struct ParseException : BaseException<ParseException>
    {
        using BaseException<ParseException>::BaseException;
    };
}
