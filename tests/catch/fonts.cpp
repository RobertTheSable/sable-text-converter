#include <catch2/catch.hpp>
#include <vector>
#include "font.h"

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
    static const char* parsedChars = " (),.!?\"01234567890';";
    int index = offset;
    for (char i = 'A'; i < 'Z'; i++) {
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

TEST_CASE("Test 1-byte fonts.")
{
    using sable::Font;
    std::vector<std::tuple<std::string, int, bool>> commands = {
        {"End", 0, false},
        {"NewLine", 01, true},
        {"Test", 07, false}
    };
    auto normalNode = createSampleNode(true, 1, 160, 8, commands, {"ll", "la", "e?", "ia", "❤"}, 4);
    SECTION("Test font with widths")
    {
        normalNode[Font::FONT_ADDR] = "!somewhere";
        normalNode[Font::ENCODING]["[Special]"] = 100;
        Font f(normalNode, "normal");
        REQUIRE(f);
        std::vector<int> v;
        int expectedResult = normalNode[Font::ENCODING]["A"][Font::CODE_VAL].as<int>();
        REQUIRE(std::get<0>(f.getTextCode("A")) == expectedResult);
        REQUIRE(std::get<0>(f.getTextCode("Special")) == 100);
        expectedResult = normalNode[Font::ENCODING]["ll"][Font::CODE_VAL].as<int>();
        REQUIRE(std::get<0>( f.getTextCode("l", "l")) == expectedResult);
        REQUIRE(f.getWidth("l") == normalNode[Font::ENCODING]["l"][Font::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(f.getWidth("la") == normalNode[Font::ENCODING]["la"][Font::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(f.getWidth("❤") == normalNode[Font::ENCODING]["❤"][Font::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(!std::get<1>(f.getTextCode("l", "d")));
        REQUIRE(f.getMaxEncodedValue() == 255);
        v.reserve(f.getMaxEncodedValue());
        f.getFontWidths(std::back_inserter(v));
        REQUIRE(v.size() == f.getMaxEncodedValue());
        REQUIRE(v[0] == normalNode[Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(v[74] == normalNode[Font::DEFAULT_WIDTH].as<int>());
        REQUIRE(f.getCommandValue() == 0);
        REQUIRE(f.getMaxWidth() == 160);
        REQUIRE(f.getByteWidth() == 1);
        REQUIRE_THROWS(f.getTextCode("@"));
        REQUIRE_THROWS(f.getWidth("@"));
        REQUIRE(f.getHasDigraphs());
        REQUIRE(f.getFontWidthLocation() == "!somewhere");
        REQUIRE_THROWS(f.getExtraValue("SomeExtra"));
    }
    SECTION("Font with no default width.")
    {
        normalNode.remove(Font::DEFAULT_WIDTH);
        Font f(normalNode, "normal");
        std::vector<int> v;
        v.reserve(f.getMaxEncodedValue());
        f.getFontWidths(std::back_inserter(v));
        REQUIRE(v[74] == 0);
    }
    SECTION("Test commands")
    {
        normalNode[Font::COMMANDS]["EncodingTest"] = 2;
        Font f(normalNode, "normal");
        REQUIRE(f.getEndValue() == normalNode[Font::COMMANDS]["End"]["code"].as<int>());
        REQUIRE(f.getCommandCode("Test") == normalNode[Font::COMMANDS]["Test"]["code"].as<int>());
        REQUIRE(f.getCommandCode("EncodingTest") == 2);
        REQUIRE(!f.isCommandNewline("Test"));
        REQUIRE(f.isCommandNewline("NewLine"));
        REQUIRE_THROWS(f.getCommandCode("Something"));
        REQUIRE_THROWS(f.isCommandNewline("Something"));
    }
    SECTION("Check that End command is required.")
    {
        normalNode[Font::COMMANDS].remove("End");
        REQUIRE_THROWS(Font(normalNode, "fixedWidth"));
        normalNode.remove(Font::COMMANDS);
        REQUIRE_THROWS(Font(normalNode, "fixedWidth"));
    }
    SECTION("Test font with fixed widths")
    {
        normalNode.remove(Font::DEFAULT_WIDTH);
        normalNode[Font::FIXED_WIDTH] = 8;
        normalNode[Font::ENCODING]["%"] = 4;
        Font f(normalNode, "fixedWidth");
        REQUIRE(std::get<0>(f.getTextCode("%")) == 4);
        REQUIRE(f.getWidth("A") == 8);
        REQUIRE(f.getWidth("A") == f.getWidth("%"));
        std::vector<int> v;
        v.reserve(f.getMaxEncodedValue());
        f.getFontWidths(std::back_inserter(v));
        REQUIRE(v.front() == v.back());
    }
    SECTION("Fixed width font with default width.")
    {
        normalNode[Font::FIXED_WIDTH] = "true";
        Font f(normalNode, "fixedWidth");
        REQUIRE(f.getWidth("A") == 8);
        REQUIRE(f.getWidth("A") == f.getWidth("❤"));
    }
    SECTION("Test font without digraphs")
    {
        normalNode[Font::USE_DIGRAPHS] = "false";
        Font f(normalNode, "noDigraphs");
        REQUIRE(!f.getHasDigraphs());
//        auto result = f.getTextCode("l", "l");
//        REQUIRE(std::get<0>(result) == normalNode[Font::ENCODING]["l"][Font::CODE_VAL].as<int>());
//        REQUIRE(!std::get<1>(result));
    }
    SECTION("Test font with extras.")
    {
        normalNode[Font::EXTRAS]["SomeExtra"] = 1;
        Font f(normalNode, "normal");
        REQUIRE(f.getExtraValue("SomeExtra") == 1);
        REQUIRE_THROWS(f.getExtraValue("SomeMissingExtra"));
    }
}

TEST_CASE("Test 2-byte fonts.")
{
    using sable::Font;
    std::vector<std::tuple<std::string, int, bool>> commands = {
        {"End", 0xFFFF, false},
        {"NewLine", 0xFFFD, true},
        {"Test", 0xFFFE, true}
    };
    auto menuNode = createSampleNode(true, 2, 0, 8, commands, {}, 0, -1, 0, true);
    SECTION("Test 2-byte font with")
    {
        Font f(menuNode, "menu");
        REQUIRE(f.getByteWidth() == 2);
        REQUIRE(f.getMaxWidth() == 0);
        REQUIRE(f.getFontWidthLocation().empty());
        REQUIRE(f.getCommandValue() == -1);
        REQUIRE(f.isCommandNewline("Test"));
        REQUIRE(f.getEndValue() == 0xFFFF);
    }
}

TEST_CASE("Test config validation")
{
    using sable::Font;
    std::vector<std::tuple<std::string, int, bool>> commands = {
        {"End", 0, false},
        {"NewLine", 01, true},
        {"Test", 07, false}
    };
    auto normalNode = createSampleNode(true, 1, 160, 8, commands, {});
    using Catch::Matchers::Contains;
    std::string reqMessage = "Required field \"";
    SECTION("Test Digraph validation.")
    {
        normalNode[Font::USE_DIGRAPHS] = "invalid";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("must be true or false."));
    }
    SECTION("Check integer validation.")
    {
        normalNode[Font::DEFAULT_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("must be a scalar integer."));
    }
    SECTION("Check bytewidth validation.")
    {
        normalNode[Font::BYTE_WIDTH] = 3;
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("must be 1 or 2."));
        normalNode.remove(Font::BYTE_WIDTH);
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains(reqMessage + Font::BYTE_WIDTH +"\" is missing."));
    }
    SECTION("Check command value validation.")
    {
        normalNode[Font::CMD_CHAR] = "invalid";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[Font::FIXED_WIDTH] = "invalid";
        normalNode.remove(Font::DEFAULT_WIDTH);
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains(std::string("Field \"") + Font::FIXED_WIDTH + "\" must be a scalar integer."));
        normalNode[Font::DEFAULT_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains(std::string("Field \"") + Font::DEFAULT_WIDTH + "\" must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[Font::MAX_CHAR] = "invalid";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains(std::string("Field \"") + Font::MAX_CHAR + "\" must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[Font::MAX_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains(std::string("Field \"") + Font::MAX_WIDTH + "\" must be a scalar integer."));
    }
    SECTION("Check font width location validation.")
    {
        normalNode[Font::FONT_ADDR].push_back(1);
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains(std::string("Field \"") + Font::FONT_ADDR + "\" must be a string."));
    }
    SECTION("Check Encoding validation.")
    {
        normalNode[Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL] = "test";
        REQUIRE_THROWS_WITH(Font(normalNode, "test"), Contains("must be an integer."));
        normalNode[Font::ENCODING]["A"][Font::CODE_VAL] = "test";
        REQUIRE_THROWS_WITH(Font(normalNode, "test"), Contains("must be an integer."));
        normalNode[Font::ENCODING]["A"] = "test";
        REQUIRE_THROWS_WITH(Font(normalNode, "test"), Contains("must be an integer."));
        normalNode.remove(Font::ENCODING);
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("is missing."));
        normalNode[Font::ENCODING] = "1";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("must be a map."));
    }
    SECTION("Check Commands validation.")
    {
        normalNode[Font::COMMANDS]["NewLine"]["newline"] = YAML::Load("[1, 2, 3]");
        REQUIRE_THROWS_WITH(Font(normalNode, "test"), Contains("must be a scalar."));
        normalNode[Font::COMMANDS]["NewLine"] = "test";
        REQUIRE_THROWS_WITH(Font(normalNode, "test"), Contains("must be an integer."));
        normalNode.remove(Font::COMMANDS);
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("is missing."));
        normalNode[Font::COMMANDS] = "1";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("must be a map."));
    }
    SECTION("Check Extras validation.")
    {
        normalNode[Font::EXTRAS] = "1";
        REQUIRE_THROWS_WITH(Font(normalNode, ""), Contains("must be a map."));
    }
}
