#include "helpers.h"
#include "font/font.h"
#include "font/builder.h"


YAML::Node sable_tests::createSampleNode(
        bool digraphs,
        unsigned int byteWidth,
        unsigned int maxWidth,
        unsigned int defaultWidth,
        const std::vector<CommandSample> &commands,
        const std::vector<std::string> &extras,
        unsigned int skip,
        int command,
        unsigned int offset,
        bool fixedWidth
    ) {
    using sable::Font;
    YAML::Node sample;
    sample[Font::USE_DIGRAPHS] = digraphs ? "true" : "false";
    sample[Font::BYTE_WIDTH] = byteWidth;
    if (command >= 0) {
        sample[Font::CMD_CHAR] = command;
    }
    sample[Font::MAX_WIDTH] = maxWidth;
    if (fixedWidth) {
        sample[Font::FIXED_WIDTH] = defaultWidth;
    } else if (defaultWidth != 0) {
        sample[Font::DEFAULT_WIDTH] = defaultWidth;
    }
    int index = offset;
    for (char i = 'A'; i <= 'Z'; i++) {
        char lowerCase = i+0x20;
        index = offset + (i - 'A');
        sample[Font::ENCODING][i][Font::CODE_VAL] = index;
        sample[Font::ENCODING][lowerCase][Font::CODE_VAL] = index + 26;
        if (!fixedWidth) {
            auto calcLength = ((index-1) % 8) + 1;
            sample[Font::ENCODING][i][Font::TEXT_LENGTH_VAL] = calcLength;
            sample[Font::ENCODING][lowerCase][Font::TEXT_LENGTH_VAL] = calcLength;
        }
    }
    // add a duplicate entry
//    sample[Font::ENCODING]["A2"][Font::CODE_VAL] = sample[Font::ENCODING]["A"][Font::CODE_VAL].Scalar();
//    if (!fixedWidth) {
//        sample[Font::ENCODING]["A2"][Font::TEXT_LENGTH_VAL] =
//                sample[Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL].Scalar();
//    }
    index = offset + 52;
    for (int var = 0; var < sizeof (parsedChars); ++var) {
        sample[Font::ENCODING][sable_tests::parsedChars[var]][Font::CODE_VAL] = index++;
        if (!fixedWidth) {
            sample[Font::ENCODING][sable_tests::parsedChars[var]][Font::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
        }
    }
    if (!extras.empty()) {
        index += skip;
        for (auto& extra: extras) {
            sample[Font::ENCODING][extra][Font::CODE_VAL] = index++;
            if (!fixedWidth) {
                if (extra.length() == 2 &&
                        sample[Font::ENCODING][extra[0]].IsDefined() &&
                        sample[Font::ENCODING][extra[1]].IsDefined()
                        ) {
                    sample[Font::ENCODING][extra][Font::TEXT_LENGTH_VAL] =
                            sample[Font::ENCODING][extra[0]][Font::TEXT_LENGTH_VAL].as<int>() +
                            sample[Font::ENCODING][extra[1]][Font::TEXT_LENGTH_VAL].as<int>() - 1;
                } else {
                    sample[Font::ENCODING][extra][Font::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
                }
            }
        }
    }
    for (auto& cmd: commands) {
        sample[Font::COMMANDS][cmd.name][Font::CODE_VAL] = cmd.code;
        if (CommandSample::enabled(cmd.newline)) {
            sample[Font::COMMANDS][cmd.name][Font::CMD_NEWLINE_VAL] = "true";
        }
        sample[Font::COMMANDS][cmd.name][Font::CMD_PREFIX] = cmd.prefix;
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
                    {"End", 0, CommandSample::NewLine::No, "Yes"},
                    {"NewLine", sable_tests::NEWLINE_VAL, CommandSample::NewLine::Yes, "Yes"},
                    {"Test", 07, CommandSample::NewLine::No, "Yes"}
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
    sampleNode["normal"][sable::Font::EXTRAS]["Extra2"] = 2;
    sampleNode["nodigraph"][sable::Font::ENCODING] =  sampleNode["normal"][sable::Font::ENCODING];
    sampleNode["nodigraph"][sable::Font::COMMANDS] =  sampleNode["normal"][sable::Font::COMMANDS];
    sampleNode["menu"] = sable_tests::createSampleNode(
                true,
                2,
                0,
                8,
                {
                    {"End", 0xFFFF, CommandSample::NewLine::No, "No"},
                    {"NewLine", 0xFFFD, CommandSample::NewLine::Yes, "No"},
                    {"Test", 0xFFFE, CommandSample::NewLine::No, "No"}
                },
                {},
                0,
                -1,
                0,
                true
                );
    sampleNode["offset"] = sable_tests::createSampleNode(
        true,
        1,
        160,
        8,
        {
            {"End", 0, CommandSample::NewLine::No, "No"},
            {"NewLine", sable_tests::NEWLINE_VAL, CommandSample::NewLine::Yes, "No"},
            {"TestP", 17, CommandSample::NewLine::No, "Yes"}
        },
        {"ll", "la", "e?", "[special]", "❤", "e†"},
        0,
        0,
        17
    );
    sampleNode["offset"][sable::Font::EXTRAS]["Extra1"] = 1;
    sampleNode["offset"][sable::Font::EXTRAS]["Extra2"] = 2;
    return sampleNode;
}

namespace YAML {
using sable_tests::EncNode, sable_tests::NounNode;
Node convert<sable_tests::EncNode>::encode(const EncNode& rhs)
{
    using sable::Font;
    Node node;
    if (!rhs.scalar) {
        node[Font::CODE_VAL] = rhs.code;
        node[Font::TEXT_LENGTH_VAL] = rhs.length;
    } else {
        node = rhs.code;
    }
    return node;
}

Node convert<NounNode>::encode(const NounNode& rhs)
{
    using sable::Font;
    Node node;
    node[Font::CODE_VAL] = rhs.codes;
    node[Font::TEXT_LENGTH_VAL] = rhs.length;
    return node;
}

bool YAML::convert<std::map<std::string, sable::Font>>::decode(const Node &node, std::map<std::string, sable::Font> &rhs)
{
    for (auto it = node.begin(); it != node.end(); ++it) {
        rhs[it->first.Scalar()] = sable::FontBuilder::make(
            it->second,
            it->first.Scalar(),
            "en_US.UTF8"
        );
    }
    return true;
}

}

std::map<std::string, sable::Font> sable_tests::getSampleFonts()
{
    return sable_tests::getSampleNode().as<std::map<std::string, sable::Font>>();
}
