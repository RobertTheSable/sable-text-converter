#ifndef SABLE_TEST_H
#define SABLE_TEST_H

#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>

#include "font/font.h"

namespace sable_tests {

constexpr const char* defaultLocale ="en_US.utf8";

struct CommandSample { //NOLINT
    std::string name;
    int code;
    enum class NewLine {
        Yes, No
    } newline;
    std::string prefix;
    template <class E>
    inline static bool enabled(E opt) {
        return opt == E::Yes;
    }
};

YAML::Node createSampleNode(
        bool digraphs,
        unsigned int byteWidth,
        unsigned int maxWidth,
        unsigned int defaultWidth,
        const std::vector<CommandSample>& commands,
        const std::vector<std::string>& extras = {},
        unsigned int skip = 0,
        int command = 0,
        unsigned int offset = 1,
        bool fixedWidth = false
);

YAML::Node getSampleNode();
std::locale getTestLocale();

std::map<std::string, sable::Font> getSampleFonts();

struct EncNode {
    std::string code;
    std::string length;
    bool scalar = false;
};

struct NounNode {
    std::vector<std::string> codes;
    std::string length;
};

};

namespace YAML {
template <>
struct convert<sable_tests::EncNode> {
    static YAML::Node encode(const sable_tests::EncNode& rhs);
};

template <>
struct convert<sable_tests::NounNode> {
    static Node encode(const sable_tests::NounNode& rhs);
};


template <>
struct convert<std::map<std::string, sable::Font>> {
     static bool decode(const Node& node, std::map<std::string, sable::Font>& rhs);
};
};
#endif
