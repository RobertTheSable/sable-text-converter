#include "font.h"
#include <exception>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "exceptions.h"
#include "localecheck.h"

//TODO: Throw an error here if an encoding has a code or width of 0

namespace sable {

Font::Font(
    const std::string& name,
    const std::locale& normalizationLocale,
    bool hasDigraphs,
    int commandCode,
    bool isFixedWidth,
    int defaultWidth,
    int maxWidth,
    const std::string& fontWidthLocation,
    int glyphByteLength
) {
    m_Name = name;
    m_NormLocale = normalizationLocale;
    m_HasDigraphs = hasDigraphs;
    m_CommandValue = commandCode;
    m_DefaultWidth = defaultWidth;
    m_IsFixedWidth = isFixedWidth;
    m_MaxWidth = maxWidth;
    m_FontWidthLocation = fontWidthLocation;
    m_ByteWidth = glyphByteLength;
}

    const Font::CommandNode &Font::getCommandData(const std::string &id) const
    {
        if (auto cIt = m_CommandConvertMap.find(normalize(m_NormLocale, id));
            cIt == m_CommandConvertMap.end()
        ) {
            throw CodeNotFound("Command not found.");
        } else {
            return cIt->second;
        }
    }

    void Font::addCommandData(const std::string &id, CommandNode &&data)
    {
        auto nId = normalize(m_NormLocale, id);
        if (nId == "End") {
            endValue = data.code;
        }
        m_CommandConvertMap[normalize(m_NormLocale, id)] = data;
    }


    void Font::addExtra(const std::string &id, int value)
    {
        m_Extras[normalize(m_NormLocale, id)] = value;
    }

    bool Font::isCommandNewline(const std::string &id) const
    {
        return getCommandData(id).isNewLine;
    }

    unsigned int Font::getCommandCode(const std::string &id) const
    {
        return getCommandData(id).code;
    }

    std::optional<Font::TextNode> Font::lookupTextNode(int page, const std::string &id, bool throws) const
    {
        if (!(page < m_Pages.size())) {
            throw CodeNotFound(std::string("font " + m_Name + " does not have page " + std::to_string(page)));
        }

        auto realId = normalize(m_NormLocale, id);

        const auto &m_TextConvertMap = m_Pages[page].glyphs;
        if (auto it = m_TextConvertMap.find(realId); it == m_TextConvertMap.end()) {
            if (!throws) {
                return std::nullopt;
            }
            throw CodeNotFound(std::string("\"") + id + "\" not found in " + ENCODING + " of font " + m_Name);
        } else {
            auto tmp = it->second;
            return tmp;
        }
    }

    int Font::getWidth(int page, const std::string &id) const
    {
       auto wVal = lookupTextNode(page, id, true)->width;
       if (m_IsFixedWidth || wVal <= 0) {
           return m_DefaultWidth;
       } else {
           return wVal;
       }
    }

    std::tuple<unsigned int, bool> Font::getTextCode(int page, const std::string &id, const std::string& next) const
    {
        if (!next.empty()) {
            if (auto dg = lookupTextNode(page, id + next, false); (bool)dg) {
                return std::make_tuple(dg->code, true);
            }
        }
        return std::make_tuple(lookupTextNode(page, id, true)->code, false);
    }

    CharacterIterator Font::getNounData(int page, const std::string &id) const
    {
        if (!(page < m_Pages.size())) {
            throw CodeNotFound(std::string("font " + m_Name + " does not have page " + std::to_string(page)));
        }
        auto nounItr = m_Pages[page].nouns.find(normalize(m_NormLocale, id));
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

    int Font::getExtraValue(const std::string &id) const
    {
        if (auto eIt = m_Extras.find(normalize(m_NormLocale, id)); eIt == m_Extras.end()) {
            throw CodeNotFound(id + " not found in font " + m_Name);
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
        int index = 0;
        if (this->getCommandValue() == 0) {
            index = 1;
        }
        auto t = v.begin();
        if (m_IsFixedWidth) {
            while (index <= m_Pages[page].maxValue) {
                *(inserter++) = m_DefaultWidth;
                index++;
                ++t;
            }
        } else {
            // never ran into an issue with this until I started testing pages
            // my best guess is that adding those cases somehow messed with memory enough
            // to trigger the infinite loop condition.
            unsigned int lastCode = m_Pages[page].maxValue+1;
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
            for ( ; index <= m_Pages[page].maxValue; ++index) {
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

    int Font::getCommandValue() const
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

#ifdef SABLE_KEEP_DEPRECATED
    std::tuple<unsigned int, bool> Font::getTextCode(const std::string &id, const std::string& next) const
    {
        return getTextCode(0, id, next);
    }

    CharacterIterator Font::getNounData(const std::string &id) const
    {
        return getNounData(0, id);
    }

    void Font::getFontWidths(std::back_insert_iterator<std::vector<int> > inserter) const
    {
        getFontWidths(0, inserter);
    }

    int Font::getWidth(const std::string &id) const
    {
        return getWidth(0, id);
    }

    int Font::getMaxEncodedValue() const
    {
        return getMaxEncodedValue(0);
    }
#endif
}

