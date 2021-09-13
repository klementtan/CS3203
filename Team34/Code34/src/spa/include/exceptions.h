// exceptions.h

#pragma once

#include <zpr.h>
#include <util.h>

#include <exception>

namespace util
{
    template <typename SubType>
    struct BaseException : public std::exception
    {
        std::string message;

        template <typename... Args>
        BaseException(const char* who, const char* fmt, const Args&... args)
        {
            util::log(zpr::sprint("{} exception", who).c_str(), fmt, args...);
            this->message = zpr::sprint(fmt, args...);
        }

        const char* what() const noexcept
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

#if 0
    struct PqlException : public std::exception
    {
        std::string message;

    public:
        template <typename... Args>
        PqlException(const char* who, const char* fmt, const Args&... args)
        {
            util::log(zpr::sprint("{} Exception", who).c_str(), fmt, args...);
            this->message = zpr::sprint(fmt, args...);
        }

        const char* what() const throw()
        {
            return message.c_str();
        }
    };
#endif
}
