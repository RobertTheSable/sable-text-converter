#include "helpers.h"
#include "font.h"

YAML::Node sable_tests::createSampleNode(bool digraphs, unsigned int byteWidth, unsigned int maxWidth, unsigned int defaultWidth, const std::vector<std::tuple<std::string, int, bool> > &commands, const std::vector<std::string> &extras, unsigned int skip, int command, unsigned int offset, bool fixedWidth)
{
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
    static const char* parsedChars = " (),.!?\"01234567890';";
    int index = offset;
    for (char i = 'A'; i <= 'Z'; i++) {
        char lowerCase = i+0x20;
        index = offset + (i - 'A');
        sample[Font::ENCODING][i][Font::CODE_VAL] = index;
        sample[Font::ENCODING][lowerCase][Font::CODE_VAL] = index + 26;
        if (!fixedWidth) {
            sample[Font::ENCODING][i][Font::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
            sample[Font::ENCODING][lowerCase][Font::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
        }
    }
    index = offset + 52;
    for (int var = 0; var < 21; ++var) {
        sample[Font::ENCODING][parsedChars[var]][Font::CODE_VAL] = index++;
        if (!fixedWidth) {
            sample[Font::ENCODING][parsedChars[var]][Font::TEXT_LENGTH_VAL] = ((index-1) % 8) + 1;
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
        sample[Font::COMMANDS][std::get<0>(cmd)][Font::CODE_VAL] = std::get<1>(cmd);
        if (std::get<2>(cmd)) {
            sample[Font::COMMANDS][std::get<0>(cmd)][Font::CMD_NEWLINE_VAL] = "true";
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
                {"ll", "la", "e?", "[special]", "❤"}
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
