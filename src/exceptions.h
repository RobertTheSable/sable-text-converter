#ifndef SABLE_EXCEPTIONS_H
#define SABLE_EXCEPTIONS_H

#include <stdexcept>
#include <yaml-cpp/mark.h>
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

    class FontError : public std::runtime_error {
    public:
        FontError(const YAML::Mark& mark, const std::string &name, const std::string& field, const std::string& msg = "");
        FontError(const YAML::Mark& mark, const std::string &name, const std::string& field, const std::string& subField, const std::string& msg);
        YAML::Mark getMark() const;
        std::string getField() const;
        std::string getMessage() const;
        std::string getName() const;

    private:
        static const std::string buildWhat(const YAML::Mark &mark, const std::string &name, const std::string &field, const std::string &msg, const std::string& subField = "");
        YAML::Mark m_Mark;
        std::string m_Name, m_Field, m_Message;
    };
}
#endif // EXCEPTIONS_H
