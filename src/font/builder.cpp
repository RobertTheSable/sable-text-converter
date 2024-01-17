#include "builder.h"
#include "normalize.h"
#include "codenotfound.h"

namespace sable {

using Builder = FontBuilder;

template<typename ...Args>
FontError generateError(YAML::Mark m, Args ...args)
{
    return FontError(m.line, args...);
}


enum class SubfieldErrorType {
    Integer,
    IntegerSequence,
    FollowConstraint
};
std::string getErrorTypeLabel(SubfieldErrorType type)
{
    using Type = SubfieldErrorType;
    switch (type) {
    case Type::Integer:
        return "an integer.";
    case Type::IntegerSequence:
        return "a sequence of integers.";
    case Type::FollowConstraint:
        return "in alignment with a constraint.";
    default:
        throw std::logic_error("Undefined option.");
    }
}


template<class T>
struct SubfieldError : std::runtime_error {
    std::string fieldName;
    std::string parentName;
    using Type = SubfieldErrorType;
    Type type;
    std::string constraint;
    SubfieldError(std::string fieldName, Type type):
        constraint{getErrorTypeLabel(type)},
        std::runtime_error(fieldName + " must be " + getErrorTypeLabel(type)) {
        this->fieldName = fieldName;
        this->type = type;
    }
    SubfieldError(std::string fieldName, std::string constraint):
        type{Type::FollowConstraint},
        std::runtime_error(fieldName + ' ' + constraint) {
        this->fieldName = fieldName;
        this->constraint = constraint;
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
        throw generateError(e.mark, name, field);
    } catch (YAML::TypedBadConversion<std::string> &e) {
        throw generateError(e.mark, name, field, "a string.");
    } catch (YAML::TypedBadConversion<int> &e) {
        throw generateError(e.mark, name, field, "a scalar integer.");
    } catch (std::runtime_error& e) {
        throw generateError(node.Mark(), name, field, e.what());
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
            throw generateError(node.Mark(), name, field, "a map.");
        } else {
            for (auto it = node.begin(); it != node.end(); ++it) {
                std::string str = it->first.as<std::string>();

                try {
                    adder(str, it->second.as<T>());
                } catch (YAML::TypedBadConversion<T>) {
                    throw generateError(node.Mark(), name, field, str, msg);
                } catch (sable::SubfieldError<T> &e) {
                    e.parentName = str;
                    throw e;
                }
            }
        }
    } catch (YAML::InvalidNode &e) {
        throw generateError(e.mark, name, field);
    }
}


Font::Page buildPage(
    const std::string& name,
    const YAML::Node& node,
    int commandValue,
    int glyphWidth
) {
    Font::Page page;

    auto encodingConv = [&page, commandValue] (const std::string& id, Font::TextNode&& n) {
        if (n.code == commandValue) {
            throw SubfieldError<Font::TextNode>(Font::CODE_VAL, "matches the command value, which is not allowed.");
        }
        std::string newId = id;
        if (newId.front() == '[') {
            newId.erase(0,1);
        }
        if (newId.back() == ']') {
            newId.pop_back();
        }
        newId = normalize(newId);
        page.addGlyph(newId, std::move(n));
    };
    if (node.IsMap() && node[Font::ENCODING].IsDefined()) {
        try {
            generate<Font::TextNode>(name, node[Font::ENCODING], Font::ENCODING, encodingConv);
        } catch (ConvertError &e) {
            // encoding conversion *should* not throw one of these
            // but better safe than sorry
            throw generateError(e.mark, name, e.field, e.type);
        } catch (sable::SubfieldError<Font::TextNode>& e) {
            if (e.type == SubfieldErrorType::FollowConstraint) {
                throw generateError(
                    node.Mark(),
                    name,
                    Font::ENCODING,
                    e.parentName,
                    std::string("") + "has a \"" + e.fieldName + "\" field that " + e.constraint
                );
            }
            throw generateError(
                node.Mark(),
                name,
                Font::ENCODING,
                e.parentName,
                std::string("") + "has a \"" + e.fieldName + "\" field that is not "  + e.constraint
            );
        }

        if (node[Font::NOUNS].IsDefined()) {
            try {
                auto conv = [&page] (const std::string& id, Font::NounNode&& n) {
                    std::string newId = id;
                    if (newId.front() == '[') {
                        newId.erase(0,1);
                    }
                    if (newId.back() == ']') {
                        newId.pop_back();
                    }
                    newId = normalize(newId);
                    page.addNoun(newId, std::move(n));
                };
                generate<Font::NounNode>(name, node[Font::NOUNS], Font::NOUNS, conv);
            } catch (sable::SubfieldError<sable::Font::NounNode> &e) {
                if (e.fieldName == Font::CODE_VAL) {
                    throw generateError(
                        node.Mark(),
                        name,
                        Font::NOUNS,
                        e.parentName,
                        std::string("") + "must have a \"" + Font::CODE_VAL + "\" field that is a sequence of integers."
                    );
                } else {
                    throw generateError(
                        node.Mark(),
                        name,
                        Font::NOUNS,
                        e.parentName,
                        std::string("") + "has a \"" + Font::TEXT_LENGTH_VAL + "\" field that is not an integer."
                    );
                }
            } catch (ConvertError &e) {
                // noun conversion *should* not throw one of these
                // but better safe than sorry
                throw generateError(e.mark, name, e.field, e.type);
            }
        }
    } else if (node.IsMap()) {
        try {
            generate<Font::TextNode>(
                name,
                node,
                Font::PAGES,
                encodingConv
            );
        } catch (ConvertError &e) {
            // should not occur
            throw generateError(e.mark, name, Font::PAGES, e.field, e.type);
        }
    } else {
        throw generateError(node.Mark(), name, Font::PAGES, "a sequence of maps.");
    }
    if (node[Font::MAX_CHAR].IsDefined()) {
        page.setMaxValue(validate<int>(node[Font::MAX_CHAR], name, Font::MAX_CHAR));
    } else {
        page.setMaxValue(0);
        if (glyphWidth == 1) {
            page.setMaxValue(0xFF);
        } else if (glyphWidth == 2) {
            page.setMaxValue(0xFFFF);
        } else {
            // should be unreachable
            throw std::logic_error("Only fonts of width 1 or 2 are allowed.");
        }
    }
    return page;
}

ConvertError::ConvertError(std::string f, std::string t, YAML::Mark m):
    std::runtime_error(f + " must be " + t), field(f), type(t), mark(m) {};


Font Builder::make(const YAML::Node &config, const std::string &name, const std::string& localeId)
{

    bool hasDigraphs = config[Font::USE_DIGRAPHS].IsDefined() ? (validate<std::string>(config[Font::USE_DIGRAPHS], name, Font::USE_DIGRAPHS, [] (const std::string& val) {
        if (val != "true" && val != "false") {
            throw std::runtime_error("true or false.");
        }
        return val;
    }) == "true") : false;
    auto commandCode = config[Font::CMD_CHAR].IsDefined() ?
                std::optional<unsigned int>{validate<int>(config[Font::CMD_CHAR], name, Font::CMD_CHAR)} :
                std::nullopt;
    auto isFixedWidth = config[Font::FIXED_WIDTH].IsDefined();
    auto defaultwidth = [&config, isFixedWidth, &name]() {
        if (config[Font::DEFAULT_WIDTH].IsDefined()) {
            return validate<int>(config[Font::DEFAULT_WIDTH], name, Font::DEFAULT_WIDTH);
        } else if (isFixedWidth) {
            return validate<int>(config[Font::FIXED_WIDTH], name, Font::FIXED_WIDTH);
        } else {
            return 0;
        }
    }();
    auto maxWidth = config[Font::MAX_WIDTH].IsDefined() ? validate<int>(config[Font::MAX_WIDTH], name, Font::MAX_WIDTH) : 0;
    auto fontWidthLocation = config[Font::FONT_ADDR].IsDefined() ? validate<std::string>(config[Font::FONT_ADDR], name, Font::FONT_ADDR) : "";
    auto byteWidth = validate<int>(config[Font::BYTE_WIDTH], name, Font::BYTE_WIDTH, [] (const int& val) {
        if (val != 1 && val != 2) {
            throw std::runtime_error("1 or 2.");
        }
        return val;
    });

    Font f(
        name,
        localeId,
        hasDigraphs,
        commandCode,
        isFixedWidth,
        defaultwidth,
        maxWidth,
        fontWidthLocation,
        byteWidth
    );

    if (!config[Font::ENCODING].IsDefined()) {
        throw generateError(config.Mark(), name, Font::ENCODING, "is missing.");
    }
    f.addPage(buildPage(name, config, commandCode.value_or(-1), byteWidth));

    if (config[Font::PAGES].IsDefined()) {
        if (!config[Font::PAGES].IsSequence()) {
            throw generateError(config[Font::PAGES].Mark(), name, Font::PAGES, "a sequence of maps.");
        }
        int pageIdx = 1;
        for (YAML::Node page: config[Font::PAGES]) {
            try {
                f.addPage(buildPage(name, page, commandCode.value_or(-1), byteWidth));
            } catch (sable::SubfieldError<Font::TextNode>& e) {
                if (e.type == SubfieldErrorType::FollowConstraint) {
                    throw generateError(
                        page.Mark(),
                        name,
                        std::string{Font::PAGES} + " #" + std::to_string(pageIdx),
                        e.parentName,
                        std::string("") + "has a \"" + e.fieldName + "\" field that "  + e.constraint
                    );
                }
                throw generateError(
                    page.Mark(),
                    name,
                    std::string{Font::PAGES} + " #" + std::to_string(pageIdx),
                    e.parentName,
                    std::string("") + "has a \"" + e.fieldName + "\" field that is not "  + e.constraint
                );
            }
            ++pageIdx;
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
        throw generateError(e.mark, name, e.field, e.type);
    }
    if (config[Font::EXTRAS].IsDefined()) {
        try {
            auto conv = [&f] (const std::string& id, int i) {
                f.addExtra(id, i);
            };
            generate<int>(name, config[Font::EXTRAS], Font::EXTRAS, conv);
        } catch (YAML::TypedBadConversion<int> &e) {
            // probably unreachable
            throw generateError(e.mark, name, Font::EXTRAS, "scalar integers.");
        }
    }
    for (auto cd : {"End", "NewLine"}) {
        try {
             f.getCommandData(cd);
        } catch (CodeNotFound &e) {
            throw generateError(config[Font::COMMANDS].Mark(), name, cd, "defined in the Commands section.");
        }
    }
    f.validate(true);

    return f;
}

} // namespace sable

namespace YAML {
using sable::Font;
    bool convert<sable::Font::TextNode>::decode(const Node& node, sable::Font::TextNode& rhs)
    {
        if (node.IsMap() && node[Font::CODE_VAL].IsDefined()) {
            try {
                rhs.code = node[Font::CODE_VAL].as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::SubfieldError<Font::TextNode>(Font::CODE_VAL, sable::SubfieldErrorType::Integer);
            }
            if (node[Font::TEXT_LENGTH_VAL].IsDefined()) {
                try {
                    rhs.width = node[Font::TEXT_LENGTH_VAL].as<int>();
                } catch(YAML::TypedBadConversion<int> &e) {
                    throw sable::SubfieldError<Font::TextNode>(Font::TEXT_LENGTH_VAL, sable::SubfieldErrorType::Integer);
                }
            }
            return true;
        } else if (node.IsScalar()) {
            try {
                rhs.code = node.as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::SubfieldError<Font::TextNode>(Font::CODE_VAL, sable::SubfieldErrorType::Integer);
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
            if (auto cNode = node[Font::CODE_VAL]; !cNode.IsScalar()) {
                throw sable::ConvertError(Font::CODE_VAL, "an integer.", cNode.Mark());
            } else {
                try {
                    rhs.code = node[Font::CODE_VAL].as<unsigned int>();
                } catch(YAML::TypedBadConversion<unsigned int> &e) {
                    throw sable::ConvertError(Font::CODE_VAL, "an integer.", cNode.Mark());
                }
            }
            if (auto nlNode = node[Font::CMD_NEWLINE_VAL]; nlNode.IsDefined()) {
                if (!nlNode.IsScalar()) {
                    throw sable::ConvertError(Font::CMD_NEWLINE_VAL, "a scalar.", nlNode.Mark());
                }
                rhs.isNewLine = true;
            }

            if (node[Font::CMD_PAGE].IsDefined()) {
                rhs.page = node[Font::CMD_PAGE].as<int>();
            }
            if (auto prefixNode = node[Font::CMD_PREFIX]; !prefixNode.IsDefined()) {
                rhs.isPrefixed = true;
            } else if (prefixNode.IsScalar()) {
                auto val = prefixNode.Scalar();
                std::transform(val.begin(), val.end(), val.begin(), ::tolower);
                if (val == "yes" || val == "true" || val == "") {
                    rhs.isPrefixed = true;
                } else if (val == "no" || val == "false") {
                    rhs.isPrefixed = false;
                } else {
                    throw sable::ConvertError(Font::CMD_PREFIX, "yes, true, no, or false.", prefixNode.Mark());
                }
            } else {
                throw sable::ConvertError(Font::CMD_PREFIX, "a scalar.", prefixNode.Mark());
            }
            return true;
        } else if (node.IsScalar()) {
            try {
                rhs.code = node.as<unsigned int>();
            } catch(YAML::TypedBadConversion<unsigned int> &e) {
                throw sable::ConvertError(Font::CODE_VAL, "an integer.", node.Mark());
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
        if (node[Font::TEXT_LENGTH_VAL].IsDefined()) {
            try {
               rhs.width = node[Font::TEXT_LENGTH_VAL].as<int>();
            } catch (YAML::TypedBadConversion<int> &e) {
                throw sable::SubfieldError<sable::Font::NounNode>{Font::TEXT_LENGTH_VAL, sable::SubfieldErrorType::Integer};
            }
        }
        if (!node[Font::CODE_VAL].IsDefined() || !node[Font::CODE_VAL].IsSequence()) {
            throw sable::SubfieldError<sable::Font::NounNode>{Font::CODE_VAL, sable::SubfieldErrorType::IntegerSequence};
        }
        rhs.codes.reserve(node[Font::CODE_VAL].size());
        for (auto it2 = node[Font::CODE_VAL].begin(); it2 != node[Font::CODE_VAL].end(); ++it2) {
            try {
                rhs.codes.push_back(it2->as<int>());
            } catch (YAML::TypedBadConversion<int> &e) {
                throw sable::SubfieldError<sable::Font::NounNode>{Font::CODE_VAL, sable::SubfieldErrorType::IntegerSequence};
            }
        }
        return true;
    }

}
