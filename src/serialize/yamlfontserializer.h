#ifndef SABLE_YAMLFONTSERIALIZER_H
#define SABLE_YAMLFONTSERIALIZER_H

#include <yaml-cpp/yaml.h>
#include <locale>
#include "font/font.h"

namespace sable {

class YamlFontSerializer
{
public:
    static constexpr const char* USE_DIGRAPHS = "HasDigraphs";
    static constexpr const char* BYTE_WIDTH = "ByteWidth";
    static constexpr const char* CMD_CHAR = "CommandValue";
    static constexpr const char* FIXED_WIDTH = "FixedWidth";
    static constexpr const char* DEFAULT_WIDTH = "DefaultWidth";
    static constexpr const char* MAX_CHAR = "MaxEncodedValue";
    static constexpr const char* MAX_WIDTH = "MaxWidth";
    static constexpr const char* FONT_ADDR = "FontWidthAddress";
//    static constexpr const char* ENCODING = "Encoding";
//    static constexpr const char* COMMANDS = "Commands";
//    static constexpr const char* NOUNS = "Nouns";
//    static constexpr const char* EXTRAS = "Extras";
    static constexpr const char* CODE_VAL = "code";
    static constexpr const char* TEXT_LENGTH_VAL = "length";
    static constexpr const char* CMD_NEWLINE_VAL = "newline";
    static constexpr const char* CMD_PAGE = "page";
    static constexpr const char* PAGES = "Pages";
//    Font(const YAML::Node &config, const std::string& name, const std::locale& normalizationLocale);
    Font generateFont(const YAML::Node &config, const std::string& name, const std::locale& normalizationLocale);
private:
    Font::Page buildPage(
        const std::locale normLocale,
        const std::string& name,
        const YAML::Node& node,
        int glyphWidth
    );
};


struct ConvertError: public std::runtime_error {
    std::string field;
    std::string type;
    YAML::Mark mark;
    ConvertError(std::string field, std::string type, YAML::Mark mark);
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

} // namespace sable


namespace YAML {
    template <>
    struct convert<sable::Font::TextNode> {
        static bool decode(const Node& node, sable::Font::TextNode& rhs);
    };
    template <>
    struct convert<sable::Font::CommandNode> {
        static bool decode(const Node& node, sable::Font::CommandNode& rhs);
    };
    template <>
    struct convert<sable::Font::NounNode> {
        static bool decode(const Node& node, sable::Font::NounNode& rhs);
    };
}

#endif // SABLE_YAMLFONTSERIALIZER_H
