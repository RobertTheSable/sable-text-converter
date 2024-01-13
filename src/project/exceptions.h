#ifndef SABLE_EXCEPTIONS_H
#define SABLE_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace sable {
    class ParseError: public std::runtime_error {
    public:
        ParseError(std::string message);
    };
    class ASMError: public std::runtime_error {
    public:
        ASMError(std::string message);
    };
    class ConfigError: public std::runtime_error {
    public:
        ConfigError(std::string message);
    };
}
#endif // EXCEPTIONS_H
