#include "font.h"
#include <exception>
#include <algorithm>

#include "normalize.h"

//TODO: Throw an error here if an encoding has a code or width of 0

namespace sable {

Font::Font(
    const std::string& name,
    const std::string& localeId,
    bool hasDigraphs,
    std::optional<unsigned int> commandCode,
    bool isFixedWidth,
    int defaultWidth,
    int maxWidth,
    const std::string& fontWidthLocation,
    int glyphByteLength
):
    m_Name{name},
    m_LocaleId{localeId},
    m_HasDigraphs{hasDigraphs},
    m_CommandValue{commandCode},
    m_DefaultWidth{defaultWidth},
    m_IsFixedWidth{isFixedWidth},
    m_MaxWidth{maxWidth},
    m_FontWidthLocation{fontWidthLocation},
    m_ByteWidth{glyphByteLength}
{
    if (commandCode) {
        m_FirstPrefixedCommand = commandCode.value() + 1;
    } else {
        m_FirstPrefixedCommand = 0;
    }
}

    const Font::CommandNode &Font::getCommandData(const std::string &id) const
    {
        if (auto cIt = m_CommandConvertMap.find(normalize(id));
            cIt == m_CommandConvertMap.end()
        ) {
            throw CodeNotFound("Command not found.");
        } else {
            return cIt->second;
        }
    }

    void Font::addCommandData(const std::string &id, CommandNode &&data)
    {
        auto nId = normalize(id);
        if (nId == "End") {
            endValue = data.code;
        }

        if (!data.isPrefixed && (bool)m_CommandValue) {
            if (auto minPrefix = data.code + 1; minPrefix > m_FirstPrefixedCommand) {
                m_FirstPrefixedCommand = minPrefix;
            }
        }

        m_CommandConvertMap[nId] = data;
    }


    void Font::addExtra(const std::string &id, int value)
    {
        m_Extras[normalize(id)] = value;
    }

    bool Font::isCommandNewline(const std::string &id) const
    {
        return getCommandData(id).isNewLine;
    }

    unsigned int Font::getCommandCode(const std::string &id) const
    {
        return getCommandData(id).code;
    }

    std::optional<Font::TextNode> Font::lookupTextNode(int page, const std::string &id) const
    {
        if (!(page < m_Pages.size())) {
            throw CodeNotFound(std::string("font " + m_Name + " does not have page " + std::to_string(page)));
        }

        auto realId = normalize(id);

        const auto &m_TextConvertMap = m_Pages[page].glyphs;
        if (auto it = m_TextConvertMap.find(realId); it == m_TextConvertMap.end()) {
            return std::nullopt;
        } else {
            auto tmp = it->second;
            return tmp;
        }
    }

    int Font::getWidth(int page, const std::string &id) const
    {
        if (auto lv = lookupTextNode(page, id); (bool)lv) {
            if (m_IsFixedWidth || lv->width <= 0) {
                return m_DefaultWidth;
            } else {
                return lv->width;
            }
        }
        throw CodeNotFound(std::string("\"") + id + "\" not found in " + ENCODING + " of font " + m_Name);
    }

   std::optional<std::tuple<unsigned int, bool>> Font::getTextCode(int page, const std::string &id, const std::string& next) const
    {
        if (!next.empty()) {
            if (auto dg = lookupTextNode(page, id + next); (bool)dg) {
                return std::make_tuple(dg->code, true);
            }
        }
        if (auto sg = lookupTextNode(page, id); (bool)sg) {
            return std::make_tuple(sg->code, false);
        }
        return std::nullopt;
    }

    CharacterIterator Font::getNounData(int page, const std::string &id) const
    {
        if (!(page < m_Pages.size())) {
            throw CodeNotFound(std::string("font " + m_Name + " does not have page " + std::to_string(page)));
        }
        auto nounItr = m_Pages[page].nouns.find(normalize(id));
        if (nounItr == m_Pages[page].nouns.end()) {
            throw CodeNotFound(id + " not found in " + NOUNS + " of font " + m_Name);
        }
        const NounNode& noun = nounItr->second;
        int width = noun.width;
        if  (m_IsFixedWidth || width <= 0) {
            width = m_DefaultWidth * noun.codes.size();
        }
        return CharacterIterator(
            noun.codes.cbegin(),
            noun.codes.cend(),
            width
        );
    }

    std::optional<int> Font::getExtraValue(const std::string &id) const noexcept
    {
        if (auto eIt = m_Extras.find(normalize(id)); eIt == m_Extras.end()) {
            return std::nullopt;
        } else {
            return eIt->second;
        }
    }

    void Font::getFontWidths(int page, std::back_insert_iterator<std::vector<int> > inserter) const
    {
        typedef std::pair<std::string, TextNode> TextDataPair;
        if (!(page < m_Pages.size())) {
            throw std::logic_error(
                std::string("Tried to get widths for non-existent page ")  +
                        std::to_string(page) +
                        " in font " +
                        m_Name
            );
        }
        const auto& m_TextConvertMap = m_Pages[page].glyphs;
        std::vector<TextDataPair> v(m_TextConvertMap.begin(), m_TextConvertMap.end());
        std::sort(v.begin(), v.end(), [](TextDataPair& a, TextDataPair& b) {
            return a.second.code < b.second.code;
        });

        int index = 0, limit = m_Pages[page].maxValue;
        if ((bool)m_CommandValue) {
            index = m_FirstPrefixedCommand;
        }
        auto t = v.begin();
        if (m_IsFixedWidth) {
            while (index <= limit) {
                *(inserter++) = m_DefaultWidth;
                index++;
                ++t;
            }
        } else {
            // never ran into an issue with this until I started testing pages
            // my best guess is that adding those cases somehow messed with memory enough
            // to trigger the infinite loop condition.
            unsigned int lastCode = limit+1;
            while (t != v.end()) {
                if (lastCode != t->second.code ) {
                    if (index == t->second.code) {
                        if (t->second.width > 0) {
                            *(inserter++) = t->second.width;
                        } else {
                            *(inserter++) = m_DefaultWidth;
                        }
                        ++index;
                        lastCode = (t++)->second.code;
                    } else {
                        *(inserter++) = m_DefaultWidth;
                        ++index;
                    }
                } else {
                    ++t;
                }
            }
            for ( ; index <= limit; ++index) {
                *(inserter++) = m_DefaultWidth;
            }
        }

    }

    unsigned int Font::getEndValue() const
    {
        return endValue;
    }

    void Font::addPage(Page &&pg)
    {
        m_Pages.push_back(pg);
    }

    void Font::validate(bool result)
    {
        m_IsValid = result;
        return;
    }

    const std::string& Font::getFontWidthLocation() const
    {
        return m_FontWidthLocation;
    }

    int Font::getNumberOfPages() const
    {
        return m_Pages.size();
    }

    Font::operator bool() const
    {
        return m_IsValid;
    }

    int Font::getByteWidth() const
    {
        return m_ByteWidth;
    }

    std::optional<unsigned int> Font::getCommandValue() const
    {
        return m_CommandValue;
    }

    int Font::getMaxWidth() const
    {
        return m_MaxWidth;
    }

    int Font::getMaxEncodedValue(int page) const
    {
        return m_Pages[page].maxValue;
    }

    bool Font::getHasDigraphs() const
    {
        return m_HasDigraphs;
    }
}

