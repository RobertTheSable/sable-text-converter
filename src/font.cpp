#include "font.h"
#include "exceptions.h"
#include <exception>

namespace sable {

    Font::Font(const YAML::Node &config, const std::string& name) : m_IsValid(false), m_Name(name), m_YamlNodeMark(config.Mark())
        {
            m_ByteWidth = validate<int>(config[BYTE_WIDTH], BYTE_WIDTH);
            try {
                m_TextConvertMap = generateMap<TextNode>(config[ENCODING], ENCODING, [](std::string& str) {
                        if (str.front() == '[') {
                            str.erase(0,1);
                        }
                        if (str.back() == ']') {
                            str.pop_back();
                        }
                    });
            } catch(YAML::TypedBadConversion<int> &e) {
                throw FontError(e.mark, TEXT_LENGTH_VAL, "an integer.");
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw FontError(e.mark, CODE_VAL, "an integer.");
            }
            try {
                m_CommandConvertMap = generateMap<CommandNode>(config[COMMANDS], COMMANDS);
            } catch(YAML::TypedBadConversion<CommandNode> &e) {
                throw FontError(e.mark, CMD_NEWLINE_VAL, "a scalar.");
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw FontError(e.mark, CODE_VAL, "an integer.");
            }
            if (config[EXTRAS].IsDefined()) {
                try {
                    m_Extras = generateMap<int>(config[EXTRAS], EXTRAS);
                } catch(YAML::TypedBadConversion<int> &e) {
                    throw FontError(e.mark, EXTRAS, "scalar integers.");
                }
            }
            m_HasDigraphs = config[USE_DIGRAPHS].IsDefined() ? (validate<std::string>(config[USE_DIGRAPHS], USE_DIGRAPHS, [] (const std::string& val) {
                if (val != "true" && val != "false") {
                    throw std::runtime_error("true or false");
                }
                return val;
            }) == "true") : false;
            m_CommandValue = config[CMD_CHAR].IsDefined() ? validate<int>(config[CMD_CHAR], CMD_CHAR) : -1;
            m_IsFixedWidth = config[FIXED_WIDTH].IsDefined();
            m_DefaultWidth = m_IsFixedWidth ? validate<int>(config[FIXED_WIDTH], FIXED_WIDTH) : 0;
            m_MaxWidth = config[MAX_WIDTH].IsDefined() ? validate<int>(config[MAX_WIDTH], MAX_WIDTH) : 0;
            m_FontWidthLocation = config[FONT_ADDR].IsDefined() ? validate<std::string>(config[FONT_ADDR], FONT_ADDR) : "";
            if (config[MAX_CHAR].IsDefined()) {
                m_MaxEncodedValue = validate<int>(config[MAX_CHAR], MAX_CHAR);
            } else {
                m_MaxEncodedValue = 0;
                for (int i = 0; i < m_ByteWidth; i++) {
                    m_MaxEncodedValue |= (0xFF << i);
                }
            }
            m_IsValid = true;
            try {
                 endValue = m_CommandConvertMap.at("End").code;
            } catch (std::out_of_range &e) {
                throw std::runtime_error(m_Name.insert(0, "\"End\" command is missing for font "));
            }

    }

    unsigned int Font::getCommandCode(const std::string &id) const
    {
        if (m_CommandConvertMap.find(id) == m_CommandConvertMap.end()) {
            throw std::runtime_error("");
        }
        return m_CommandConvertMap.at(id).code;
    }

    std::tuple<unsigned int, bool> Font::getTextCode(const std::string &id, const std::string& next) const
    {
        if (!next.empty() && m_TextConvertMap.find(id + next) != m_TextConvertMap.end()) {
            return std::make_tuple(m_TextConvertMap.at(id + next).code, true);
        } else if (m_TextConvertMap.find(id) == m_TextConvertMap.end()) {
            throw std::runtime_error(id + " not found in " + ENCODING + " of font " + m_Name);
        }
        return std::make_tuple(m_TextConvertMap.at(id).code, false);
    }

    int Font::getExtraValue(const std::string &id) const
    {
        if (m_Extras.find(id) == m_Extras.end()) {
            throw std::runtime_error("");
        }
        return m_Extras.at(id);
    }

    int Font::getWidth(const std::string &id) const
    {
        if (m_TextConvertMap.find(id) == m_TextConvertMap.end()) {
            throw std::runtime_error(id + " not found in " + COMMANDS + " of font " + m_Name);
        } else if (m_IsFixedWidth || m_TextConvertMap.at(id).width <= 0) {
            return m_DefaultWidth;
        } else {
            return m_TextConvertMap.at(id).width;
        }
    }

    bool Font::isCommandNewline(const std::string &id) const
    {
        if (m_CommandConvertMap.find(id) == m_CommandConvertMap.end()) {
            throw std::runtime_error(id + " not found in " + EXTRAS + " of font " + m_Name);
        }
        return m_CommandConvertMap.at(id).isNewLine;
    }

    void Font::getFontWidths(std::back_insert_iterator<std::vector<int> > inserter) const
    {
        typedef std::pair<std::string, TextNode> TextDataPair;
        std::vector<TextDataPair> v(m_TextConvertMap.begin(), m_TextConvertMap.end());
        std::sort(v.begin(), v.end(), [](TextDataPair& a, TextDataPair& b) {
            return a.second.code < b.second.code;
        });
        int index = 0;
        if (this->getCommandValue() == 0) {
            index = 1;
        }
        auto t = v.begin();
        if (m_IsFixedWidth) {
            while (index < m_MaxEncodedValue) {
                *(inserter++) = m_DefaultWidth;
                index++;
            }
        }
        unsigned int lastCode = m_MaxEncodedValue+1;
        while (index < m_MaxEncodedValue) {
            if (lastCode != t->second.code) {
                if (index == t->second.code) {
                    *(inserter++) = t->second.width;
                    index++;
                    lastCode = (t++)->second.code;
                } else {
                    *(inserter++) = m_DefaultWidth;
                    index++;
                }
            }
        }
    }

    unsigned int Font::getEndValue() const
    {
        return endValue;
    }

    const std::string& Font::getFontWidthLocation() const
    {
        return m_FontWidthLocation;
    }

    Font::operator bool() const
    {
        return m_IsValid;
    }

    int Font::getByteWidth() const
    {
        return m_ByteWidth;
    }

    int Font::getCommandValue() const
    {
        return m_CommandValue;
    }

    int Font::getMaxWidth() const
    {
        return m_MaxWidth;
    }

    int Font::getMaxEncodedValue() const
    {
        return m_MaxEncodedValue;
    }

    bool Font::getHasDigraphs() const
    {
        return m_HasDigraphs;
    }
    template<class T>
    T Font::validate(const YAML::Node &&node, const std::string& field, const std::function<T (const T&)>& validator)
    {
        try {
            return validator(node.as<T>());
        } catch (YAML::InvalidNode &e) {
            throw FontError(e.mark, field);
        } catch (YAML::TypedBadConversion<std::string> &e) {
            throw FontError(e.mark, field, "a string.");
        } catch (YAML::TypedBadConversion<int> &e) {
            throw FontError(e.mark, field, "a scalar integer.");
        } catch (std::runtime_error& e) {
            throw FontError(node.Mark(), field, e.what());
        }
    }
    template<class T>
    T Font::validate(const YAML::Node &&node, const std::string& field, const std::function<T (const T&)>&& validator)
    {
        try {
            return validator(node.as<T>());
        } catch (YAML::InvalidNode &e) {
            throw FontError(e.mark, field);
        } catch (YAML::TypedBadConversion<std::string> &e) {
            throw FontError(e.mark, field, "a string.");
        } catch (YAML::TypedBadConversion<int> &e) {
            throw FontError(e.mark, field, "a scalar integer.");
        } catch (const char*& e) {
            throw FontError(node.Mark(), field, e);
        }
    }

    FontError::FontError(const YAML::Mark &mark, const std::string &field, const std::string &msg) :
        std::runtime_error(buildWhat(mark, field, msg)), m_Mark(mark), m_Field(field), m_Message(msg) {}

    YAML::Mark FontError::getMark() const
    {
        return m_Mark;
    }

    std::string FontError::getField() const
    {
        return m_Field;
    }

    std::string FontError::getMessage() const
    {
        return m_Message;
    }

    const std::string FontError::buildWhat(const YAML::Mark &mark, const std::string &field, const std::string &msg)
    {
        std::stringstream message;
        if (!msg.empty()) {
            message << "Line " << mark.line << ": " << "Field \"" << field << "\" must be " << msg;
        } else {
            message << "Required field \"" << field << "\" is missing.";
        }
        return message.str();
    }

    template<class T>
    std::unordered_map<std::string, T> Font::generateMap(const YAML::Node &&node, const std::string& field, const std::function<void (std::string&)>&& formatter)
    {
        std::unordered_map<std::string, T> map;
        try {
            if(!node.IsMap()) {
                throw FontError(node.Mark(), field, "a map");
            } else {
                for (auto it = node.begin(); it != node.end(); ++it) {
                    std::string str = it->first.as<std::string>();
                    formatter(str);
                    map[str] = it->second.as<T>();
                }
            }
            return map;
        } catch (YAML::InvalidNode &e) {
            throw FontError(e.mark, field);
        }
    }
}
namespace YAML {
    bool convert<sable::Font::TextNode>::decode(const Node& node, sable::Font::TextNode& rhs)
    {
        if (node.IsMap() && node[sable::Font::CODE_VAL].IsDefined()) {
            rhs.code = node[sable::Font::CODE_VAL].as<unsigned int>();
            if (node[sable::Font::TEXT_LENGTH_VAL].IsDefined()) {
                rhs.width = node[sable::Font::TEXT_LENGTH_VAL].as<int>();
            }
            return true;
        } else if (node.IsScalar()) {
            rhs.code = node.as<unsigned int>();
            return true;
        }
        return false;
    }
    bool convert<sable::Font::CommandNode>::decode(const Node& node, sable::Font::CommandNode& rhs)
    {
        if (node.IsMap() && node[sable::Font::CODE_VAL].IsDefined()) {
            rhs.code = node[sable::Font::CODE_VAL].as<unsigned int>();
            if (node[sable::Font::CMD_NEWLINE_VAL].IsDefined()) {
                if (!node[sable::Font::CMD_NEWLINE_VAL].IsScalar()) {
                    return false;
                }
                rhs.isNewLine = true;
            }
            return true;
        } else if (node.IsScalar()) {
            rhs.code = node.as<unsigned int>();
            return true;
        }
        return false;
    }
}
