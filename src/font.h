#ifndef TEXTTYPE_H
#define TEXTTYPE_H

#include <yaml-cpp/yaml.h>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <functional>
#include <optional>
#include <cctype>
#include <locale>
#include "characteriterator.h"

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
        static constexpr const char* NOUNS = "Nouns";
        static constexpr const char* EXTRAS = "Extras";
        static constexpr const char* CODE_VAL = "code";
        static constexpr const char* TEXT_LENGTH_VAL = "length";
        static constexpr const char* CMD_NEWLINE_VAL = "newline";
        static constexpr const char* CMD_PAGE = "page";
        static constexpr const char* PAGES = "Pages";
        Font()=default;
        Font(const YAML::Node &config, const std::string& name, const std::locale& normalizationLocale);
        Font(Font&&)=default;
        Font(Font&)=default;
        Font(const Font&)=default;
        Font& operator=(const Font&)=default;
        int getByteWidth() const;
        int getCommandValue() const;
        int getMaxWidth() const;
        bool getHasDigraphs() const;
        const std::string& getFontWidthLocation() const;

        int getNumberOfPages() const;

        struct CommandNode {
            unsigned int code;
            int page;
            bool isNewLine = false;
        };

        const CommandNode& getCommandData(const std::string& id) const;

        unsigned int getCommandCode(const std::string& id) const;
        bool isCommandNewline(const std::string& id) const;

        int getMaxEncodedValue(int page) const;
        [[deprecated ("Use getMaxEncodedValue(int page) instead.")]]
        int getMaxEncodedValue() const;

        [[deprecated("Use getTextCode(int page, const std::string& id, ...) instead.")]]
        std::tuple<unsigned int, bool> getTextCode(const std::string& id, const std::string& next = "") const;
        std::tuple<unsigned int, bool> getTextCode(int page, const std::string& id, const std::string& next = "") const;

        [[deprecated("Use getWidth(int page, const std::string& id) instead.")]]
        int getWidth(const std::string& id) const;
        int getWidth(int page, const std::string& id) const;

        [[deprecated("Use getNounData(int page, const std::string& id) instead.")]]
        CharacterIterator getNounData(const std::string& id) const;
        CharacterIterator getNounData(int page, const std::string& id) const;

        int getExtraValue(const std::string& id) const;

        [[deprecated("Use getFontWidths(int page, ...) instead.")]]
        void getFontWidths(std::back_insert_iterator<std::vector<int>> inserter) const;
        void getFontWidths(int page, std::back_insert_iterator<std::vector<int>> inserter) const;

        explicit operator bool() const;

    private:
        //TODO use pImpl to hide most if this
        struct TextNode {
            unsigned int code;
            int width = 0;
        };
        struct NounNode {
            std::vector<int> codes;
            int width = 0;
        };
        struct Page {
            std::unordered_map<std::string, TextNode> glyphs;
            std::unordered_map<std::string, NounNode> nouns;
            int maxValue;
        };

        Page buildPage(
            const YAML::Node& n
        );

        std::locale m_NormLocale;
        YAML::Mark m_YamlNodeMark;
        std::string m_Name;
        bool m_IsValid, m_HasDigraphs, m_IsFixedWidth;
        int m_ByteWidth, m_CommandValue, m_MaxWidth, m_DefaultWidth;
        unsigned int endValue;
        std::string m_FontWidthLocation;
        std::vector<Page> m_Pages;
        std::unordered_map<std::string, CommandNode> m_CommandConvertMap;
        std::unordered_map<std::string, int> m_Extras;
        template <class T>
        using Validator = std::optional<std::function<T (const T&)>>;

        template <class T>
        T validate(const YAML::Node&& node, const std::string& field, const Validator<T>& validator = std::nullopt);

        template <class T>
        std::unordered_map<std::string, T> generateMap(
            const YAML::Node&& node,
            const std::string& field,
            const std::optional<std::function<void (std::string&)>>& formatter = std::nullopt,
            const Validator<T> validator = std::nullopt
        );
        std::unordered_map<std::string, NounNode> generateNouns(
            const YAML::Node&& node,
            const std::string& field
        );
        std::optional<TextNode> lookupTextNode(int page, const std::string& id, bool throwsIfCodeMissing) const;
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
