#ifndef TEXTTYPE_H
#define TEXTTYPE_H

#include <yaml-cpp/yaml.h>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <functional>
#include <cctype>

namespace sable {
    class Font
    {
    public:
        enum ReadType {ERROR, TEXT, COMMAND, EXTRA};
        static constexpr const char* USE_DIGRAPHS = "HasDigraphs";
        static constexpr const char* BYTE_WIDTH = "ByteWidth";
        static constexpr const char* CMD_CHAR = "CommandValue";
        static constexpr const char* FIXED_WIDTH = "FixedWidth";
        static constexpr const char* DEFAULT_WIDTH = "DefaultWidth";
        static constexpr const char* MAX_CHAR = "MaxEncodedValue";
        static constexpr const char* MAX_WIDTH = "MaxWidth";
        static constexpr const char* FONT_ADDR = "FontWidthAddress";
        static constexpr const char* ENCODING = "Encoding";
        static constexpr const char* COMMANDS = "Commands";
        static constexpr const char* EXTRAS = "Extras";
        static constexpr const char* CODE_VAL = "code";
        static constexpr const char* TEXT_LENGTH_VAL = "length";
        static constexpr const char* CMD_NEWLINE_VAL = "newline";
        Font()=default;
        Font(const YAML::Node &config, const std::string& name);
        Font& operator=(Font&&) =default;

        int getByteWidth() const;
        int getCommandValue() const;
        int getMaxWidth() const;
        int getMaxEncodedValue() const;
        bool getHasDigraphs() const;
        const std::string& getFontWidthLocation() const;

        unsigned int getCommandCode(const std::string& id) const;
        std::tuple<unsigned int, bool> getTextCode(const std::string& id, const std::string& next = "") const;
        int getExtraValue(const std::string& id) const;
        int getWidth(const std::string& id) const;
        bool isCommandNewline(const std::string& id) const;
        void getFontWidths(std::back_insert_iterator<std::vector<int>> inserter) const;

        explicit operator bool() const;

    private:
        struct TextNode {
            unsigned int code;
            int width = 0;
        };
        struct CommandNode {
            unsigned int code;
            bool isNewLine = false;
        };
        YAML::Mark m_YamlNodeMark;
        std::string m_Name;
        bool m_IsValid, m_HasDigraphs, m_IsFixedWidth;
        int m_ByteWidth, m_CommandValue, m_MaxWidth, m_MaxEncodedValue, m_DefaultWidth;
        unsigned int endValue;
        std::string m_FontWidthLocation;
        std::unordered_map<std::string, TextNode> m_TextConvertMap;
        std::unordered_map<std::string, CommandNode> m_CommandConvertMap;
        std::unordered_map<std::string, int> m_Extras;
        template <class T>
        T validate(const YAML::Node&& node, const std::string& field, const std::function<T (const T&)>& validator);
        template <class T>
        T validate(const YAML::Node&& node, const std::string& field, const std::function<T (const T&)>&& validator = [](const T& val){return val;});
        template <class T>
        std::unordered_map<std::string, T> generateMap(const YAML::Node&& node, const std::string& field, const std::function<void (std::string&)>&& formatter = [](std::string& str){});
    public:
        friend YAML::convert<sable::Font::TextNode>;
        friend YAML::convert<sable::Font::CommandNode>;
        unsigned int getEndValue() const;
    };
}

namespace YAML {
    template <>
    struct convert<sable::Font::TextNode> {
        static bool decode(const Node& node, sable::Font::TextNode& rhs);
    };
    template <>
    struct convert<sable::Font::CommandNode> {
        static bool decode(const Node& node, sable::Font::CommandNode& rhs);
    };
}

#endif // TEXTTYPE_H
