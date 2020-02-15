#ifndef SABLE_EXCEPTIONS_H
#define SABLE_EXCEPTIONS_H

#include <stdexcept>
#include <yaml-cpp/mark.h>
#include <string>

namespace sable {
    class FontError : public std::runtime_error {
    public:
        FontError(const YAML::Mark& mark, const std::string& field, const std::string& msg = "");
        YAML::Mark getMark() const;
        std::string getField() const;
        std::string getMessage() const;
    private:
        static const std::string buildWhat(const YAML::Mark &mark, const std::string &field, const std::string &msg);
        YAML::Mark m_Mark;
        std::string m_Field, m_Message;
    };
}
#endif // EXCEPTIONS_H
