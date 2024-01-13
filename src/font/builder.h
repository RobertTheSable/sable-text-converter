#ifndef SABLE_FONT_BUILDER_H
#define SABLE_FONT_BUILDER_H

#include <string>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include "font.h"

namespace sable {

struct FontBuilder
{
    static Font make(const YAML::Node &config, const std::string& name, const std::string& localeId);
}; // namespace Font

struct ConvertError: public std::runtime_error {
    std::string field;
    std::string type;
    YAML::Mark mark;
    ConvertError(std::string field, std::string type, YAML::Mark mark);
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


#endif // SABLE_FONT_BUILDER_H
