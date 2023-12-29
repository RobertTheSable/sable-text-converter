#include "font.h"

#include <sstream>

namespace sable {

FontError::FontError(int line_, const std::string &name, const std::string &field, const std::string &msg) :
    std::runtime_error(buildWhat(line_, name, field, msg)), line{line_}, m_Name(name), m_Field(field), m_Message(msg) {}
FontError::FontError(int line_, const std::string &name, const std::string &field, const std::string& subField, const std::string &msg) :
    std::runtime_error(buildWhat(line_, name, field, msg, subField)), line{line_}, m_Name(name), m_Field(field+'>'+subField), m_Message(msg) {}

std::string FontError::getField() const
{
    return m_Field;
}

std::string FontError::getMessage() const
{
    return m_Message;
}

std::string FontError::getName() const
{
    return m_Name;
}


const std::string FontError::buildWhat(int line, const std::string &name, const std::string &field, const std::string &msg, const std::string& subField)
{
    std::stringstream message;
    message << "In font \"" + name + '"';
    if (field.empty()) {
        message << ", line " << line << ": ";
        if (msg.empty()) {
            message << "Unknown error.";
        } else {
            message << msg;
        }
    } else if (!msg.empty()) {
        if (!subField.empty()) {
            message << ", line " << line << ": Field \"" + field + "\" has invalid entry \"" << subField +"\"" +
                       (msg.empty() ? "" : ": " + msg);
        } else {
            message << ", line " << line << ": Field \"" + field + "\" must be " + msg;
        }
    } else {
        message << ": Required field \"" << field << "\" is missing.";
    }
    return message.str();
}


}
