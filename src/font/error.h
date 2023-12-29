#ifndef FONT_ERROR_H
#define FONT_ERROR_H

#include <stdexcept>
#include <string>

namespace sable {

class FontError : public std::runtime_error {
public:
    FontError(int line, const std::string &name, const std::string& field, const std::string& msg = "");
    FontError(int line, const std::string &name, const std::string& field, const std::string& subField, const std::string& msg);
    std::string getField() const;
    std::string getMessage() const;
    std::string getName() const;

private:
    static const std::string buildWhat(const int line, const std::string &name, const std::string &field, const std::string &msg, const std::string& subField = "");
    int line;
    std::string m_Name, m_Field, m_Message;
};

}

#endif // ERROR_H
