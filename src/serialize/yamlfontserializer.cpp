#include "yamlfontserializer.h"
#include "exceptions.h"
#include "localecheck.h"
#include "font/font.h"

namespace sable {

template<class T>
struct SubfieldError : std::runtime_error {
    std::string fieldName;
    std::string parentName;
    std::string type;
    SubfieldError(std::string fieldName, std::string type): std::runtime_error(fieldName + " must be a " + type) {
        this->fieldName = fieldName;
        this->type = type;
    }
};

template <class T>
using Validator = std::optional<std::function<T (const T&)>>;

template <class T>
T validate(const YAML::Node&& node, const std::string& name, const std::string& field, const Validator<T>& validator = std::nullopt) {
    try {
        if (validator == std::nullopt) {
            return [](const T& val){return val;}(node.as<T>());
        } else {
            return (*validator)(node.as<T>());
        }
    } catch (YAML::InvalidNode &e) {
        throw FontError(e.mark, name, field);
    } catch (YAML::TypedBadConversion<std::string> &e) {
        throw FontError(e.mark, name, field, "a string.");
    } catch (YAML::TypedBadConversion<int> &e) {
        throw FontError(e.mark, name, field, "a scalar integer.");
    } catch (std::runtime_error& e) {
        throw FontError(node.Mark(), name, field, e.what());
    }
}

template <class T, class Ad>
void generate(
    const std::string& name,
    const YAML::Node& node,
    const std::string& field,
    Ad adder,
    const std::string& msg = "must define a numeric code."
) {
    try {
        if(!node.IsMap()) {
            throw FontError(node.Mark(), name, field, "a map.");
        } else {
            for (auto it = node.begin(); it != node.end(); ++it) {
                std::string str = it->first.as<std::string>();
                if (str == "length") {
                    int test = 1;
                }
                try {
                    adder(str, it->second.as<T>());
                } catch (YAML::TypedBadConversion<T>) {
                    throw FontError(node.Mark(), name, field, str, msg);
                } catch (ConfigError &e) {
                    throw FontError(node.Mark(), name, "", e.what());
                } catch (sable::SubfieldError<T> &e) {
                    e.parentName = str;
                    throw e;
                }
            }
        }
    } catch (YAML::InvalidNode &e) {
        throw FontError(e.mark, name, field);
    }
}

Font::Page YamlFontSerializer::buildPage(
    const std::locale normLocale,
    const std::string& name,
    const YAML::Node& node,
    int glyphWidth
) {
    Font::Page page;

    auto encodingConv = [&page, &locale=normLocale] (const std::string& id, Font::TextNode&& n) {
        std::string newId = id;
        if (newId.front() == '[') {
            newId.erase(0,1);
        }
        if (newId.back() == ']') {
            newId.pop_back();
        }
        newId = normalize(locale, newId);
        page.addGlyph(newId, std::move(n));
    };
    if (node[Font::ENCODING].IsDefined()) {
        try {
            generate<Font::TextNode>(name, node[Font::ENCODING], Font::ENCODING, encodingConv);
        } catch (ConvertError &e) {
            throw FontError(e.mark, name, e.field, e.type);
        }

        if (node[Font::NOUNS].IsDefined()) {
            try {
                auto conv = [&page, &locale=normLocale] (const std::string& id, Font::NounNode&& n) {
                    std::string newId = id;
                    if (newId.front() == '[') {
                        newId.erase(0,1);
                    }
                    if (newId.back() == ']') {
                        newId.pop_back();
                    }
                    newId = normalize(locale, newId);
                    page.addNoun(newId, std::move(n));
                };
                generate<Font::NounNode>(name, node[Font::NOUNS], Font::NOUNS, conv);
            } catch (sable::SubfieldError<sable::Font::NounNode> &e) {
                if (e.fieldName == CODE_VAL) {
                    throw FontError(
                        node.Mark(),
                        name,
                        Font::NOUNS,
                        e.parentName,
                        std::string("") + "must have a \"" + CODE_VAL + "\" field that is a sequence of integers."
                    );
                } else {
                    throw FontError(
                        node.Mark(),
                        name,
                        Font::NOUNS,
                        e.parentName,
                        std::string("") + "has a \"" + TEXT_LENGTH_VAL + "\" field that is not an integer."
                    );
                }
            } catch (ConvertError &e) {
                throw FontError(e.mark, name, e.field, e.type);
            }
        }
    } else if (node.IsMap()) {
        try {
            generate<Font::TextNode>(
                name,
                node,
                std::string("Each entry in ") + YamlFontSerializer::PAGES,
                encodingConv
            );
        } catch (ConvertError &e) {
            throw FontError(e.mark, name, e.field, e.type);
        }
    } else {
        throw FontError(node.Mark(), name, YamlFontSerializer::PAGES, "a sequence of maps.");
    }
    if (node[MAX_CHAR].IsDefined()) {
        page.setMaxValue(validate<int>(node[MAX_CHAR], name, MAX_CHAR));
    } else {
        page.setMaxValue(0);
        if (glyphWidth == 1) {
            page.setMaxValue(0xFF);
        } else if (glyphWidth == 2) {
            page.setMaxValue(0xFFFF);
        } else {
            throw std::logic_error("Only fonts of width 1 or 2 are allowed.");
        }
    }
    return page;
}

Font YamlFontSerializer::generateFont(const YAML::Node &config, const std::string &name, const std::locale &normalizationLocale)
{
    bool hasDigraphs = config[USE_DIGRAPHS].IsDefined() ? (validate<std::string>(config[USE_DIGRAPHS], name, USE_DIGRAPHS, [] (const std::string& val) {
        if (val != "true" && val != "false") {
            throw std::runtime_error("true or false.");
        }
        return val;
    }) == "true") : false;
    auto commandCode = config[CMD_CHAR].IsDefined() ? validate<int>(config[CMD_CHAR], name, CMD_CHAR) : -1;
    auto isFixedWidth = config[FIXED_WIDTH].IsDefined();
    auto defaultwidth = [&config, isFixedWidth, &name]() {
        if (config[DEFAULT_WIDTH].IsDefined()) {
            return validate<int>(config[DEFAULT_WIDTH], name, DEFAULT_WIDTH);
        } else if (isFixedWidth) {
            return validate<int>(config[FIXED_WIDTH], name, FIXED_WIDTH);
        } else {
            return 0;
        }
    }();
    auto maxWidth = config[MAX_WIDTH].IsDefined() ? validate<int>(config[MAX_WIDTH], name, MAX_WIDTH) : 0;
    auto fontWidthLocation = config[FONT_ADDR].IsDefined() ? validate<std::string>(config[FONT_ADDR], name, FONT_ADDR) : "";
    auto byteWidth = validate<int>(config[BYTE_WIDTH], name, BYTE_WIDTH, [] (const int& val) {
        if (val != 1 && val != 2) {
            throw std::runtime_error("1 or 2.");
        }
        return val;
    });
    auto m_NormLocale = normalizationLocale;

    Font f(
        name,
        normalizationLocale,
        hasDigraphs,
        commandCode,
        isFixedWidth,
        defaultwidth,
        maxWidth,
        fontWidthLocation,
        byteWidth
    );

    if (!config[Font::ENCODING].IsDefined()) {
        throw FontError(config.Mark(), name, Font::ENCODING, "is missing.");
    }
    f.addPage(buildPage(normalizationLocale, name, config, byteWidth));

    if (config[PAGES].IsDefined()) {
        if (!config[PAGES].IsSequence()) {
            throw FontError(config[PAGES].Mark(), name, PAGES, "a seuqence of maps.");
        }
        for (YAML::Node page: config[PAGES]) {
            f.addPage(buildPage(normalizationLocale, name, page, byteWidth));
        }
    }

    try {
        // I'd like to validate page command configs here
        // but there's no way to do that and allow commands to be mirrored.
        auto conv = [&f] (const std::string& id, Font::CommandNode&& c) {
            f.addCommandData(id, std::move(c));
        };
        generate<Font::CommandNode>(name, config[Font::COMMANDS], Font::COMMANDS, conv);
    } catch (ConvertError &e) {
        throw FontError(e.mark, name, e.field, e.type);
    }
    if (config[Font::EXTRAS].IsDefined()) {
        try {
            auto conv = [&f] (const std::string& id, int i) {
                f.addExtra(id, i);
            };
            generate<int>(name, config[Font::EXTRAS], Font::EXTRAS, conv);
        } catch (YAML::TypedBadConversion<int> &e) {
            throw FontError(e.mark, name, Font::EXTRAS, "scalar integers.");
        }
    }
    for (auto cd : {"End", "NewLine"}) {
        try {
             f.getCommandCode(cd);
        } catch (CodeNotFound &e) {
            throw FontError(config[Font::COMMANDS].Mark(), name, cd, "defined in the Commands section.");
        }
    }
    f.validate(true);

    return f;
}

ConvertError::ConvertError(std::string f, std::string t, YAML::Mark m):
    std::runtime_error(f + " must be " + t), field(f), type(t), mark(m) {};



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


} // namespace sable

namespace YAML {
    auto CODE_VAL = sable::YamlFontSerializer::CODE_VAL;
    auto TEXT_LENGTH_VAL = sable::YamlFontSerializer::TEXT_LENGTH_VAL;
    auto CMD_NEWLINE_VAL = sable::YamlFontSerializer::CMD_NEWLINE_VAL;
    auto CMD_PAGE = sable::YamlFontSerializer::CMD_PAGE;
    bool convert<sable::Font::TextNode>::decode(const Node& node, sable::Font::TextNode& rhs)
    {
        using sable::Font;
        if (node.IsMap() && node[CODE_VAL].IsDefined()) {
            try {
                rhs.code = node[CODE_VAL].as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(CODE_VAL, "an integer.", e.mark);
            }
            if (node[TEXT_LENGTH_VAL].IsDefined()) {
                try {
                    rhs.width = node[TEXT_LENGTH_VAL].as<int>();
                } catch(YAML::TypedBadConversion<int> &e) {
                    throw sable::ConvertError(TEXT_LENGTH_VAL, "an integer.", e.mark);
                }
            }
            return true;
        } else if (node.IsScalar()) {
            try {
                rhs.code = node.as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(CODE_VAL, "an integer.", e.mark);
            }
            return true;
        }
        return false;
    }
    bool convert<sable::Font::CommandNode>::decode(const Node& node, sable::Font::CommandNode& rhs)
    {
        using sable::Font;
        rhs.page = -1;
        if (node.IsMap() && node[CODE_VAL].IsDefined()) {
            try {
                rhs.code = node[CODE_VAL].as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(CODE_VAL, "an integer.", e.mark);
            }
            if (node[CMD_NEWLINE_VAL].IsDefined()) {
                try {
                    node[CMD_NEWLINE_VAL].as<std::string>();
                } catch(YAML::TypedBadConversion<std::string> &e) {
                   throw sable::ConvertError(CMD_NEWLINE_VAL, "a scalar.", e.mark);
                }
                rhs.isNewLine = true;
            }

            if (node[CMD_PAGE].IsDefined()) {
                rhs.page = node[CMD_PAGE].as<int>();
            }
            return true;
        } else if (node.IsScalar()) {
            try {
                rhs.code = node.as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(CODE_VAL, "an integer.", e.mark);
            }
            return true;
        }
        return false;
    }

    bool YAML::convert<sable::Font::NounNode>::decode(const Node &node, sable::Font::NounNode &rhs)
    {
        if (!node.IsMap()) {
            return false;
        }
        if (node[TEXT_LENGTH_VAL].IsDefined()) {
            try {
               rhs.width = node[TEXT_LENGTH_VAL].as<int>();
            } catch (YAML::TypedBadConversion<int> &e) {
                throw sable::SubfieldError<sable::Font::NounNode>{TEXT_LENGTH_VAL, "an integer."};
            }
        }
        if (!node[CODE_VAL].IsDefined() || !node[CODE_VAL].IsSequence()) {
            throw sable::SubfieldError<sable::Font::NounNode>{CODE_VAL, "a sequence of integers."};
        }
        rhs.codes.reserve(node[CODE_VAL].size());
        for (auto it2 = node[CODE_VAL].begin(); it2 != node[CODE_VAL].end(); ++it2) {
            try {
                rhs.codes.push_back(it2->as<int>());
            } catch (YAML::TypedBadConversion<int> &e) {
                throw sable::SubfieldError<sable::Font::NounNode>{CODE_VAL, "a sequence of integers."};
            }
        }
        return true;
    }

}
