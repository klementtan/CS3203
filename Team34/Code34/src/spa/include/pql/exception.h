#include <zpr.h>
#include <zst.h>
#include <util.h>

#include <exception>

namespace pql::exception
{
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
} // pql::exception
