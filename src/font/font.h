#ifndef TEXTTYPE_H
#define TEXTTYPE_H

#include <unordered_map>
#include <string>
#include <stdexcept>
#include <functional>
#include <optional>
#include <cctype>
#include <iterator>

#include "characteriterator.h"
#include "error.h"
#include "codenotfound.h"

namespace sable {
    class Font
    {
    public:
        friend class FontBuilder;

        static constexpr const char* USE_DIGRAPHS = "HasDigraphs";
        static constexpr const char* BYTE_WIDTH = "ByteWidth";
        static constexpr const char* CMD_CHAR = "CommandValue";
        static constexpr const char* FIXED_WIDTH = "FixedWidth";
        static constexpr const char* DEFAULT_WIDTH = "DefaultWidth";
        static constexpr const char* MAX_CHAR = "MaxEncodedValue";
        static constexpr const char* MAX_WIDTH = "MaxWidth";
        static constexpr const char* FONT_ADDR = "FontWidthAddress";
        static constexpr const char* MIN_PREFIX_VAL = "FirstPrefixedCommand";
        static constexpr const char* MAX_PREFIX_VAL = "LastUnprefixedCommand";

        static constexpr const char* ENCODING = "Encoding";
        static constexpr const char* COMMANDS = "Commands";
        static constexpr const char* NOUNS = "Nouns";
        static constexpr const char* EXTRAS = "Extras";

        static constexpr const char* CODE_VAL = "code";
        static constexpr const char* TEXT_LENGTH_VAL = "length";
        static constexpr const char* CMD_NEWLINE_VAL = "newline";
        static constexpr const char* CMD_PREFIX = "prefix";
        static constexpr const char* CMD_PAGE = "page";
        static constexpr const char* PAGES = "Pages";

//        enum ReadType {ERROR, TEXT, COMMAND, EXTRA};
        Font(
            const std::string& name,
            const std::string& localeId,
            bool hasDigraphs,
            std::optional<unsigned int> commandCode,
            bool isFixedWidth,
            int defaultWidth,
            int maxWidth,
            const std::string& fontWidthLocation,
            int glyphByteLength
        );
        Font(const std::string& name, int byteWidth);
        Font()=default;
        Font(Font&&)=default;
        Font(Font&)=default;
        Font(const Font&)=default;
        Font& operator=(const Font&)=default;

        struct CommandNode {
            unsigned int code;
            int page;
            bool isNewLine = false;
            bool isPrefixed = true;
        };
        struct TextNode {
            unsigned int code;
            int width = 0;
        };
        struct NounNode {
            std::vector<int> codes;
            int width = 0;
        };
        class Page  {
            friend class Font;
            std::unordered_map<std::string, TextNode> glyphs;
            std::unordered_map<std::string, NounNode> nouns;
            int maxValue;
        public:
            void addGlyph(const std::string& id, TextNode&& tx) {
                glyphs[id] = tx;
            }
            void addNoun(const std::string& id, NounNode&& n) {
                nouns[id] = n;
            }
            void setMaxValue(int mx) {
                maxValue = mx;
            }
        };

        const CommandNode& getCommandData(const std::string& id) const;
        void addCommandData(const std::string& id, CommandNode&& data);

        [[deprecated("Use getCommandData(const std::string& id).code instead.")]]
        unsigned int getCommandCode(const std::string& id) const;
        [[deprecated("Use getCommandData(const std::string& id).isNewLine instead.")]]
        bool isCommandNewline(const std::string& id) const;

        std::optional<int> getExtraValue(const std::string& id) const noexcept;
        void addExtra(const std::string& id, int value);

        std::optional<std::tuple<unsigned int, bool>> getTextCode(int page, const std::string& id, const std::string& next = "") const;
        CharacterIterator getNounData(int page, const std::string& id) const;


        int getMaxEncodedValue(int page) const;
        int getWidth(int page, const std::string& id) const;
        void getFontWidths(int page, std::back_insert_iterator<std::vector<int>> inserter) const;

        int getByteWidth() const;
        std::optional<unsigned int> getCommandValue() const;
        int getMaxWidth() const;
        bool getHasDigraphs() const;
        const std::string& getFontWidthLocation() const;
        int getNumberOfPages() const;

        explicit operator bool() const;
    private:
        std::string m_LocaleId;
        std::string m_Name;
        bool m_IsValid, m_HasDigraphs, m_IsFixedWidth;
        int m_ByteWidth, m_MaxWidth, m_DefaultWidth;
        unsigned int endValue, m_FirstPrefixedCommand;
        std::optional<unsigned int> m_CommandValue;
        std::string m_FontWidthLocation;
        std::vector<Page> m_Pages;
        std::unordered_map<std::string, CommandNode> m_CommandConvertMap;
        std::unordered_map<std::string, int> m_Extras;

        std::optional<TextNode> lookupTextNode(int page, const std::string& id) const;
    public:
        unsigned int getEndValue() const;
        void addPage(Page&& pg);
        void validate(bool result);
    };
}

#endif // TEXTTYPE_H
