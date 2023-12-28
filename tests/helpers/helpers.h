#ifndef SABLE_TEST_H
#define SABLE_TEST_H

#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>
#include "parse/fontlist.h"

namespace sable_tests {
YAML::Node createSampleNode(
        bool digraphs,
        unsigned int byteWidth,
        unsigned int maxWidth,
        unsigned int defaultWidth,
        const std::vector<std::tuple<std::string, int, bool>>& commands,
        const std::vector<std::string>& extras = {},
        unsigned int skip = 0,
        int command = 0,
        unsigned int offset = 1,
        bool fixedWidth = false
);
YAML::Node getSampleNode();
std::locale getTestLocale();



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
struct convert<sable::FontList> {
     static bool decode(const Node& node, sable::FontList& rhs);
};
};
#endif
