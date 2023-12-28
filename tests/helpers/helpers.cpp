#include "helpers.h"
#include "font/font.h"
#include "localecheck.h"
#include "serialize/yamlfontserializer.h"


namespace {
    std::locale testLocale = getLocale("en_US.UTF8");
}

YAML::Node sable_tests::createSampleNode(
        bool digraphs,
        unsigned int byteWidth,
        unsigned int maxWidth,
        unsigned int defaultWidth,
        const std::vector<std::tuple<std::string, int, bool> > &commands,
        const std::vector<std::string> &extras,
        unsigned int skip,
        int command,
        unsigned int offset,
        bool fixedWidth
    ) {
    using sable::Font, sable::YamlFontSerializer;
    YAML::Node sample;
    sample[YamlFontSerializer::USE_DIGRAPHS] = digraphs ? "true" : "false";
    sample[YamlFontSerializer::BYTE_WIDTH] = byteWidth;
    if (command >= 0) {
        sample[YamlFontSerializer::CMD_CHAR] = command;
    }
    sample[YamlFontSerializer::MAX_WIDTH] = maxWidth;
    if (fixedWidth) {
        sample[YamlFontSerializer::FIXED_WIDTH] = defaultWidth;
    } else if (defaultWidth != 0) {
        sample[YamlFontSerializer::DEFAULT_WIDTH] = defaultWidth;
    }
    static const char* parsedChars = " (),.!?\"01234567890';";
    int index = offset;
    for (char i = 'A'; i <= 'Z'; i++) {
        char lowerCase = i+0x20;
        index = offset + (i - 'A');
        sample[Font::ENCODING][i][YamlFontSerializer::CODE_VAL] = index;
        sample[Font::ENCODING][lowerCase][YamlFontSerializer::CODE_VAL] = index + 26;
        if (!fixedWidth) {
            sample[Font::ENCODING][i][YamlFontSerializer::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
            sample[Font::ENCODING][lowerCase][YamlFontSerializer::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
        }
    }
    // add a duplicate entry
//    sample[Font::ENCODING]["A2"][Font::CODE_VAL] = sample[Font::ENCODING]["A"][Font::CODE_VAL].Scalar();
//    if (!fixedWidth) {
//        sample[Font::ENCODING]["A2"][Font::TEXT_LENGTH_VAL] =
//                sample[Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL].Scalar();
//    }
    index = offset + 52;
    for (int var = 0; var < 21; ++var) {
        sample[Font::ENCODING][parsedChars[var]][YamlFontSerializer::CODE_VAL] = index++;
        if (!fixedWidth) {
            sample[Font::ENCODING][parsedChars[var]][YamlFontSerializer::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
        }
    }
    if (!extras.empty()) {
        index += skip;
        for (auto& extra: extras) {
            sample[Font::ENCODING][extra][YamlFontSerializer::CODE_VAL] = index++;
            if (!fixedWidth) {
                if (extra.length() == 2 &&
                        sample[Font::ENCODING][extra[0]].IsDefined() &&
                        sample[Font::ENCODING][extra[1]].IsDefined()
                        ) {
                    sample[Font::ENCODING][extra][YamlFontSerializer::TEXT_LENGTH_VAL] =
                            sample[Font::ENCODING][extra[0]][YamlFontSerializer::TEXT_LENGTH_VAL].as<int>() +
                            sample[Font::ENCODING][extra[1]][YamlFontSerializer::TEXT_LENGTH_VAL].as<int>() - 1;
                } else {
                    sample[Font::ENCODING][extra][YamlFontSerializer::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
                }
            }
        }
    }
    for (auto& cmd: commands) {
        sample[Font::COMMANDS][std::get<0>(cmd)][YamlFontSerializer::CODE_VAL] = std::get<1>(cmd);
        if (std::get<2>(cmd)) {
            sample[Font::COMMANDS][std::get<0>(cmd)][YamlFontSerializer::CMD_NEWLINE_VAL] = "true";
        }
    }
    return sample;
}

YAML::Node sable_tests::getSampleNode()
{
    YAML::Node sampleNode;
    sampleNode["normal"] = sable_tests::createSampleNode(
                true,
                1,
                160,
                8,
                {
                    {"End", 0, false},
                    {"NewLine", 01, true},
                    {"Test", 07, false}
                },
                {"ll", "la", "e?", "[special]", "❤", "e†"}
                );
    sampleNode["nodigraph"] = sable_tests::createSampleNode(
                false,
                1,
                160,
                8,
                {},
                {},
                0,
                0,
                1,
                true
                );
    sampleNode["normal"][sable::Font::EXTRAS]["Extra1"] = 1;
    sampleNode["nodigraph"][sable::Font::ENCODING] =  sampleNode["normal"][sable::Font::ENCODING];
    sampleNode["nodigraph"][sable::Font::COMMANDS] =  sampleNode["normal"][sable::Font::COMMANDS];
    sampleNode["menu"] = sable_tests::createSampleNode(
                true,
                2,
                0,
                8,
                {
                    {"End", 0xFFFF, false},
                    {"NewLine", 0xFFFD, true},
                    {"Test", 0xFFFE, true}
                },
                {},
                0,
                -1,
                0,
                true
                );
    return sampleNode;
}

namespace YAML {
using sable_tests::EncNode, sable_tests::NounNode;
Node convert<sable_tests::EncNode>::encode(const EncNode& rhs)
{
    using sable::Font, sable::YamlFontSerializer;
    Node node;
    if (!rhs.scalar) {
        node[YamlFontSerializer::CODE_VAL] = rhs.code;
        node[YamlFontSerializer::TEXT_LENGTH_VAL] = rhs.length;
    } else {
        node = rhs.code;
    }
    return node;
}

Node convert<NounNode>::encode(const NounNode& rhs)
{
    using sable::Font, sable::YamlFontSerializer;
    Node node;
    node[YamlFontSerializer::CODE_VAL] = rhs.codes;
    node[YamlFontSerializer::TEXT_LENGTH_VAL] = rhs.length;
    return node;
}

bool YAML::convert<sable::FontList>::decode(const Node &node, sable::FontList &rhs)
{
    sable::YamlFontSerializer slz;
    for (auto it = node.begin(); it != node.end(); ++it) {
        rhs.AddFont(
            it->first.Scalar(),
            slz.generateFont(it->second, it->first.Scalar(), sable_tests::getTestLocale())
        );
    }
    return true;
}

}

std::locale sable_tests::getTestLocale()
{
    return testLocale;
}
