#include "font.h"
#include "exceptions.h"
#include <exception>

namespace sable {

    Font::Font(const YAML::Node &config, const std::string& name) : m_YamlNodeMark(config.Mark()), m_Name(name), m_IsValid(false)
        {
            m_ByteWidth = validate<int>(config[BYTE_WIDTH], BYTE_WIDTH, [] (const int& val) {
                if (val != 1 && val != 2) {
                    throw std::runtime_error("1 or 2.");
                }
                return val;
            });

            if (!config[ENCODING].IsDefined()) {
                throw FontError(config.Mark(), m_Name, ENCODING, "is missing.");
            }
            m_Pages.push_back(buildPage(config));

            if (config[PAGES].IsDefined()) {
                if (!config[PAGES].IsSequence()) {
                    throw FontError(config[PAGES].Mark(), m_Name, PAGES, "a seuqence of maps.");
                }
                for (YAML::Node page: config[PAGES]) {
                    m_Pages.push_back(buildPage(page));
                }
            }

            try {
                // I'd like to validate page command configs here
                // but there's no way to do that and allow commands to be mirrored.
                m_CommandConvertMap = generateMap<CommandNode>(config[COMMANDS], COMMANDS);
            } catch (ConvertError &e) {
                throw FontError(e.mark, m_Name, e.field, e.type);
            }
            if (config[EXTRAS].IsDefined()) {
                try {
                    m_Extras = generateMap<int>(config[EXTRAS], EXTRAS);
                } catch (YAML::TypedBadConversion<int> &e) {
                    throw FontError(e.mark, m_Name, EXTRAS, "scalar integers.");
                }
            }
            m_HasDigraphs = config[USE_DIGRAPHS].IsDefined() ? (validate<std::string>(config[USE_DIGRAPHS], USE_DIGRAPHS, [] (const std::string& val) {
                if (val != "true" && val != "false") {
                    throw std::runtime_error("true or false.");
                }
                return val;
            }) == "true") : false;
            m_CommandValue = config[CMD_CHAR].IsDefined() ? validate<int>(config[CMD_CHAR], CMD_CHAR) : -1;
            m_IsFixedWidth = config[FIXED_WIDTH].IsDefined();
            if (config[DEFAULT_WIDTH].IsDefined()) {
                m_DefaultWidth = validate<int>(config[DEFAULT_WIDTH], DEFAULT_WIDTH);
            } else if (m_IsFixedWidth) {
                m_DefaultWidth = validate<int>(config[FIXED_WIDTH], FIXED_WIDTH);
            } else {
                m_DefaultWidth = 0;
            }
            m_MaxWidth = config[MAX_WIDTH].IsDefined() ? validate<int>(config[MAX_WIDTH], MAX_WIDTH) : 0;
            m_FontWidthLocation = config[FONT_ADDR].IsDefined() ? validate<std::string>(config[FONT_ADDR], FONT_ADDR) : "";
            if (config[MAX_CHAR].IsDefined()) {
                m_MaxEncodedValue = validate<int>(config[MAX_CHAR], MAX_CHAR);
            } else {
                m_MaxEncodedValue = 0;
                if (m_ByteWidth == 1) {
                    m_MaxEncodedValue = 0xFF;
                } else if (m_ByteWidth == 2) {
                    m_MaxEncodedValue = 0xFFFF;
                } else {
                    throw std::logic_error("Only fonts of width 1 or 2 are allowed.");
                }
            }
            m_IsValid = true;
            try {
                 endValue = m_CommandConvertMap.at("End").code;
            } catch (std::out_of_range &e) {
                throw FontError(config[COMMANDS].Mark(), m_Name, "End", "defined in the Commands section.");
            }
            if (m_CommandConvertMap.find("NewLine") == m_CommandConvertMap.end()) {
                throw FontError(config[COMMANDS].Mark(), m_Name, "NewLine", "defined in the Commands section.");
            }

    }


    const Font::CommandNode &Font::getCommandData(const std::string &id) const
    {
        if (m_CommandConvertMap.find(id) == m_CommandConvertMap.end()) {
            throw CodeNotFound("");
        }
        return m_CommandConvertMap.at(id);
    }

    bool Font::isCommandNewline(const std::string &id) const
    {
         return getCommandData(id).isNewLine;
    }

    unsigned int Font::getCommandCode(const std::string &id) const
    {
        return getCommandData(id).code;
    }

    std::tuple<unsigned int, bool> Font::getTextCode(const std::string &id, const std::string& next) const
    {
        return getTextCode(0, id, next);
    }

    std::tuple<unsigned int, bool> Font::getTextCode(int page, const std::string &id, const std::string& next) const
    {
        if (!(page < m_Pages.size())) {
            throw CodeNotFound(std::string("font " + m_Name + " does not have page " + std::to_string(page)));
        }
        const auto &m_TextConvertMap = m_Pages[page].glyphs;
        if (!next.empty() && m_TextConvertMap.find(id + next) != m_TextConvertMap.end()) {
            return std::make_tuple(m_TextConvertMap.at(id + next).code, true);
        } else if (m_TextConvertMap.find(id) == m_TextConvertMap.end()) {
            throw CodeNotFound(id + " not found in " + ENCODING + " of font " + m_Name);
        }
        return std::make_tuple(m_TextConvertMap.at(id).code, false);
    }

    CharacterIterator Font::getNounData(const std::string &id) const
    {
        return getNounData(0, id);
    }

    CharacterIterator Font::getNounData(int page, const std::string &id) const
    {
        if (!(page < m_Pages.size())) {
            throw CodeNotFound(std::string("font " + m_Name + " does not have page " + std::to_string(page)));
        }
        auto nounItr = m_Pages[page].nouns.find(id);
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
        if (m_Extras.find(id) == m_Extras.end()) {
            throw CodeNotFound(id + " not found in font " + m_Name);
        }
        return m_Extras.at(id);
    }

    int Font::getWidth(const std::string &id) const
    {
        return getWidth(0, id);
    }

    int Font::getWidth(int page, const std::string &id) const
    {
        if (!(page < m_Pages.size())) {
            throw CodeNotFound(std::string("font " + m_Name + " does not have page " + std::to_string(page)));
        }
        const auto &m_TextConvertMap = m_Pages[page].glyphs;
        if (m_TextConvertMap.find(id) == m_TextConvertMap.end()) {
            throw CodeNotFound(id + " not found in " + ENCODING + " of font " + m_Name);
        } else if (m_IsFixedWidth || m_TextConvertMap.at(id).width <= 0) {
            return m_DefaultWidth;
        } else {
            return m_TextConvertMap.at(id).width;
        }
    }

    void Font::getFontWidths(std::back_insert_iterator<std::vector<int> > inserter) const
    {
        getFontWidths(0, inserter);
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
            while (index <= m_MaxEncodedValue) {
                *(inserter++) = m_DefaultWidth;
                index++;
                ++t;
            }
        } else {
            // never ran into an issue with this until I started testing pages
            // my best guess is that adding those cases somehow messed with emory enough
            // to trigger the infinite loop condition.
            unsigned int lastCode = m_MaxEncodedValue+1;
            while (t != v.end()) {
                if (lastCode != t->second.code ) {
                    if (index == t->second.code) {
                        if (t->second.width > 0) {
                            *(inserter++) = t->second.width;
                        } else {
                            *(inserter++) = m_DefaultWidth;
                        }
                        index++;
                        lastCode = (t++)->second.code;
                    } else {
                        *(inserter++) = m_DefaultWidth;
                        index++;
                    }
                } else {
                    t++;
                }
            }
            for ( ; index <= m_MaxEncodedValue; index++) {
                *(inserter++) = m_DefaultWidth;
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

    int Font::getNumberOfPages() const
    {
        return m_Pages.size();
    }

    Font::operator bool() const
    {
        return m_IsValid;
    }

    Font::Page Font::buildPage(const YAML::Node &node)
    {
        auto formatTextMap = [](std::string& str) {
            if (str.front() == '[') {
                str.erase(0,1);
            }
            if (str.back() == ']') {
                str.pop_back();
            }
        };
        Page page;
        if (node[ENCODING].IsDefined()) {
            try {
                page.glyphs = generateMap<TextNode>(node[ENCODING], ENCODING, formatTextMap);
            } catch (ConvertError &e) {
                throw FontError(e.mark, m_Name, e.field, e.type);
            }

            if (node[NOUNS].IsDefined()) {
                page.nouns = generateNouns(node[NOUNS], NOUNS);
            }
        } else if (node.IsMap()) {
            try {
                page.glyphs = generateMap<TextNode>(
                    std::move(node),
                    std::string("Each entry in ") + PAGES,
                    formatTextMap
                );
            } catch (ConvertError &e) {
                throw FontError(e.mark, m_Name, e.field, e.type);
            }
        } else {
            throw FontError(node.Mark(), m_Name, PAGES, "a sequence of maps.");
        }
        return page;
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
        // TODO: needs to be done on a per-page basis, since the final page may have less items
        return m_MaxEncodedValue;
    }

    bool Font::getHasDigraphs() const
    {
        return m_HasDigraphs;
    }
    template<class T>
    T Font::validate(const YAML::Node &&node, const std::string& field, const Validator<T>& validator)
    {
        //
        try {
            if (validator == std::nullopt) {
                return [](const T& val){return val;}(node.as<T>());
            } else {
                return (*validator)(node.as<T>());
            }
        } catch (YAML::InvalidNode &e) {
            throw FontError(e.mark, m_Name, field);
        } catch (YAML::TypedBadConversion<std::string> &e) {
            throw FontError(e.mark, m_Name, field, "a string.");
        } catch (YAML::TypedBadConversion<int> &e) {
            throw FontError(e.mark, m_Name, field, "a scalar integer.");
        } catch (std::runtime_error& e) {
            throw FontError(node.Mark(), m_Name, field, e.what());
        }
    }

    FontError::FontError(const YAML::Mark &mark, const std::string &name, const std::string &field, const std::string &msg) :
        std::runtime_error(buildWhat(mark, name, field, msg)), m_Mark(mark), m_Name(name), m_Field(field), m_Message(msg) {}
    FontError::FontError(const YAML::Mark &mark, const std::string &name, const std::string &field, const std::string& subField, const std::string &msg) :
        std::runtime_error(buildWhat(mark, name, field, msg, subField)), m_Mark(mark), m_Name(name), m_Field(field+'>'+subField), m_Message(msg) {}

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

    std::string FontError::getName() const
    {
        return m_Name;
    }

    const std::string FontError::buildWhat(const YAML::Mark &mark, const std::string &name, const std::string &field, const std::string &msg, const std::string& subField)
    {
        std::stringstream message;
        message << "In font \"" + name + '"';
        if (field.empty()) {
            message << ", line " << mark.line << ": ";
            if (msg.empty()) {
                message << "Unknown error.";
            } else {
                message << msg;
            }
        } else if (!msg.empty()) {
            if (!subField.empty()) {
                message << ", line " << mark.line << ": Field \"" + field + "\" has invalid entry \"" << subField +"\"" +
                           (msg.empty() ? "" : ": " + msg);
            } else {
                message << ", line " << mark.line << ": Field \"" + field + "\" must be " + msg;
            }
        } else {
            message << ": Required field \"" << field << "\" is missing.";
        }
        return message.str();
    }

    template<class T>
    std::unordered_map<std::string, T> Font::generateMap(
            const YAML::Node &&node,
            const std::string& field,
            const std::optional<std::function<void (std::string&)>>& formatter,
            const Validator<T> validator
    ) {
        std::unordered_map<std::string, T> map;
        try {
            if(!node.IsMap()) {
                throw FontError(node.Mark(), m_Name, field, "a map.");
            } else {
                for (auto it = node.begin(); it != node.end(); ++it) {
                    std::string str = it->first.as<std::string>();
                    if (formatter) {
                        formatter.value()(str);
                    }
                    try {
                        if (validator != std::nullopt) {
                            map[str] = (*validator)(it->second.as<T>());
                        } else {
                            map[str] = it->second.as<T>();
                        }
                    } catch (YAML::TypedBadConversion<T>) {
                        throw FontError(node.Mark(), m_Name, field, str, "must define a numeric code.");
                    } catch (ConfigError &e) {
                        throw FontError(node.Mark(), m_Name, "", e.what());
                    }
                }
            }
            return map;
        } catch (YAML::InvalidNode &e) {
            throw FontError(e.mark, m_Name, field);
        }
    }

    std::unordered_map<std::string, Font::NounNode> Font::generateNouns(
            const YAML::Node &&node,
            const std::string &field
    ) {
        std::unordered_map<std::string, NounNode> map;
        try {
            if(!node.IsMap()) {
                throw FontError(node.Mark(), m_Name, field, "a map.");
            } else {
                for (auto it = node.begin(); it != node.end(); ++it) {
                    std::string str = it->first.as<std::string>();
                    try {
                        for (auto it = node.begin(); it != node.end(); ++it) {
                            if (!it->second.IsMap()) {
                                throw FontError(node.Mark(), m_Name, field , it->first.Scalar(), "is not a map.");
                            }
                            auto&& mapNode = it->second;
                            map[str] = [&mapNode] {
                                NounNode n;
                                if (mapNode[TEXT_LENGTH_VAL].IsDefined()) {
                                    n.width = mapNode[TEXT_LENGTH_VAL].as<int>();
                                }
                                if (!mapNode[CODE_VAL].IsDefined() || !mapNode[CODE_VAL].IsSequence()) {
                                    throw YAML::TypedBadConversion<std::vector<int>>(mapNode.Mark());
                                }
                                n.codes.reserve(mapNode[CODE_VAL].size());
                                for (auto it2 = mapNode[CODE_VAL].begin(); it2 != mapNode[CODE_VAL].end(); ++it2) {
                                    try {
                                        n.codes.push_back(it2->as<int>());
                                    } catch (YAML::TypedBadConversion<int> &e) {
                                        throw YAML::TypedBadConversion<std::vector<int>>(mapNode[CODE_VAL].Mark());
                                    }

                                }
                                return n;
                            } ();
                        }
                    } catch (YAML::TypedBadConversion<int> &e) {
                        // catching this outside of the lambda so I don't have to capture more variables
                        throw FontError(
                            node.Mark(),
                            m_Name,
                            field,
                            str,
                            std::string("") + "has a \"" + TEXT_LENGTH_VAL + "\" field that is not an integer."
                        );

                    } catch (YAML::BadConversion &e) {
                        throw FontError(
                            node.Mark(),
                            m_Name,
                            field,
                            str,
                            std::string("") + "must have a \"" + CODE_VAL + "\" field that is a sequence of integers."
                        );
                    }
                }
            }
            return map;
        } catch (YAML::InvalidNode &e) {
            throw FontError(e.mark, m_Name, field);
        }
    }

    ConvertError::ConvertError(std::string f, std::string t, YAML::Mark m):
        std::runtime_error(f + " must be " + t), field(f), type(t), mark(m) {};
}
namespace YAML {
    bool convert<sable::Font::TextNode>::decode(const Node& node, sable::Font::TextNode& rhs)
    {
        using sable::Font;
        if (node.IsMap() && node[Font::CODE_VAL].IsDefined()) {
            try {
                rhs.code = node[Font::CODE_VAL].as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(Font::CODE_VAL, "an integer.", e.mark);
            }
            if (node[Font::TEXT_LENGTH_VAL].IsDefined()) {
                try {
                    rhs.width = node[Font::TEXT_LENGTH_VAL].as<int>();
                } catch(YAML::TypedBadConversion<int> &e) {
                    throw sable::ConvertError(Font::TEXT_LENGTH_VAL, "an integer.", e.mark);
                }
            }
            return true;
        } else if (node.IsScalar()) {
            try {
                rhs.code = node.as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(Font::CODE_VAL, "an integer.", e.mark);
            }
            return true;
        }
        return false;
    }
    bool convert<sable::Font::CommandNode>::decode(const Node& node, sable::Font::CommandNode& rhs)
    {
        using sable::Font;
        rhs.page = -1;
        if (node.IsMap() && node[Font::CODE_VAL].IsDefined()) {
            try {
                rhs.code = node[Font::CODE_VAL].as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(Font::CODE_VAL, "an integer.", e.mark);
            }
            if (node[Font::CMD_NEWLINE_VAL].IsDefined()) {
                try {
                    node[Font::CMD_NEWLINE_VAL].as<std::string>();
                } catch(YAML::TypedBadConversion<std::string> &e) {
                   throw sable::ConvertError(Font::CMD_NEWLINE_VAL, "a scalar.", e.mark);
                }
                rhs.isNewLine = true;
            }

            if (node[Font::CMD_PAGE].IsDefined()) {
                rhs.page = node[Font::CMD_PAGE].as<int>();
            }
            return true;
        } else if (node.IsScalar()) {
            try {
                rhs.code = node.as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(Font::CODE_VAL, "an integer.", e.mark);
            }
            return true;
        }
        return false;
    }
}
