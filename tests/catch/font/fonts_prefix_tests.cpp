#include <catch2/catch.hpp>
#include <vector>
#include <cstring>
#include <fstream>
#include <string>
#include <functional>

#include "font/builder.h"
#include "helpers.h"

inline void prefixedFontsBaseTests(sable::Font& f)
{
    REQUIRE(f.getTextCode(0, "A"));
    REQUIRE(std::get<0>(f.getTextCode(0, "A").value()) == 16);

    REQUIRE(f.getWidth(0, "A") == 8);
    REQUIRE(!f.getCommandData("End").isPrefixed);
    REQUIRE(!f.getCommandData("NewLine").isPrefixed);

    REQUIRE(f.getCommandData("Test").isPrefixed);
    REQUIRE(f.getCommandData("NeedPrefix").isPrefixed);

    std::vector<int> widths;
    f.getFontWidths(0, std::back_inserter(widths));
    auto expectedSize = 52 + sizeof(sable_tests::parsedChars) + 4 + 5;
    REQUIRE(widths.size() == expectedSize);
    int expectedWidthsStart[] = {8,1,2,3,4,5,6,7};
    for (int i = 0; i < 8; ++i) {
        REQUIRE(widths[i] == expectedWidthsStart[i]);
    }
}

TEST_CASE("Test font with non-prefixed commands")
{
    using sable::Font, sable_tests::CommandSample;
    std::vector<CommandSample> commands = {
        {"End", 0, CommandSample::NewLine::No, "No"},
        {"NewLine", 01,  CommandSample::NewLine::Yes, "no"},
        {"Test", 18,  CommandSample::NewLine::No, "True"},
        {"NeedPrefix", 17,  CommandSample::NewLine::No, "yes"}
    };
    auto normalNode = sable_tests::createSampleNode(true, 1, 160, 8, commands, {"ll", "la", "e?", "ia", "❤"}, 4, 0, 16);
    normalNode[Font::MAX_CHAR] = normalNode[Font::ENCODING]["❤"][Font::CODE_VAL].as<int>();
    YAML::Node n;
    n[Font::CODE_VAL] = 15;
    SECTION("Value = No")
    {
        n[Font::CMD_PREFIX] = "No";
    }
    SECTION("Value = no")
    {
        n[Font::CMD_PREFIX] = "no";
    }
    SECTION("Value = False")
    {
        n[Font::CMD_PREFIX] = "False";
    }
    SECTION("Value = false")
    {
        n[Font::CMD_PREFIX] = "false";
    }

    normalNode[Font::COMMANDS]["EncodingTest"] = n;
    Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);

    REQUIRE(!f.getCommandData("EncodingTest").isPrefixed);
    prefixedFontsBaseTests(f);
}


TEST_CASE("Test font with specified min/max prefixes")
{
    using sable::Font, sable_tests::CommandSample;
    std::vector<CommandSample> commands = {
        {"End", 0, CommandSample::NewLine::No, "No"},
        {"NewLine", 01,  CommandSample::NewLine::Yes, "no"},
        {"Test", 18,  CommandSample::NewLine::No, "True"},
        {"NeedPrefix", 17,  CommandSample::NewLine::No, "yes"}
    };
    auto normalNode = sable_tests::createSampleNode(true, 1, 160, 8, commands, {"ll", "la", "e?", "ia", "❤"}, 4, 0, 16);
    normalNode[Font::MAX_CHAR] = normalNode[Font::ENCODING]["❤"][Font::CODE_VAL].as<int>();


    SECTION("First Unprefixed specified")
    {
        normalNode[Font::MIN_PREFIX_VAL] = 16;
    }
    SECTION("Last Prefixed specified")
    {
        normalNode[Font::MAX_PREFIX_VAL] = 15;
    }

    Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
    prefixedFontsBaseTests(f);
}
