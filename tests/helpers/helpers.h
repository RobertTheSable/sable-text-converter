#ifndef SABLE_TEST_H
#define SABLE_TEST_H
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>

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
};
#endif
