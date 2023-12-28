#include <catch2/catch.hpp>
#include <vector>
#include <cstring>
#include <fstream>
#include <string>
#include <functional>

#include "serialize/yamlfontserializer.h"
#include "helpers.h"

TEST_CASE("Test uninititalized font.")
{
    using sable::Font;
    Font f{};
    REQUIRE(!f);
    REQUIRE_THROWS(f.getTextCode(0, "A"));
}

TEST_CASE("Test 1-byte fonts.")
{
    using sable::Font, sable::YamlFontSerializer;
    std::vector<std::tuple<std::string, int, bool>> commands = {
        {"End", 0, false},
        {"NewLine", 01, true},
        {"Test", 07, false}
    };
    auto normalNode = sable_tests::createSampleNode(true, 1, 160, 8, commands, {"ll", "la", "e?", "ia", "❤"}, 4);
    YamlFontSerializer sz;
    SECTION("Test font with widths")
    {
        normalNode[YamlFontSerializer::FONT_ADDR] = "!somewhere";
        normalNode[Font::ENCODING]["[Special]"] = 100;
        Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
        REQUIRE(f);
        std::vector<int> v;
        int expectedResult = normalNode[Font::ENCODING]["A"][YamlFontSerializer::CODE_VAL].as<int>();
        REQUIRE(std::get<0>(f.getTextCode(0, "A")) == expectedResult);
        REQUIRE(std::get<0>(f.getTextCode(0, "Special")) == 100);
        expectedResult = normalNode[Font::ENCODING]["ll"][YamlFontSerializer::CODE_VAL].as<int>();
        REQUIRE(std::get<0>( f.getTextCode(0, "l", "l")) == expectedResult);
        REQUIRE(f.getWidth(0, "l") == normalNode[Font::ENCODING]["l"][YamlFontSerializer::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(f.getWidth(0, "la") == normalNode[Font::ENCODING]["la"][YamlFontSerializer::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(f.getWidth(0, "❤") == normalNode[Font::ENCODING]["❤"][YamlFontSerializer::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(!std::get<1>(f.getTextCode(0, "l", "d")));
        REQUIRE(f.getMaxEncodedValue(0) == 255);
        v.reserve(f.getMaxEncodedValue(0));
        f.getFontWidths(0, std::back_inserter(v));
        REQUIRE(v.size() == f.getMaxEncodedValue(0));
        REQUIRE(v[0] == normalNode[Font::ENCODING]["A"][YamlFontSerializer::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(v[74] == normalNode[YamlFontSerializer::DEFAULT_WIDTH].as<int>());
        REQUIRE(f.getCommandValue() == 0);
        REQUIRE(f.getMaxWidth() == 160);
        REQUIRE(f.getByteWidth() == 1);
        REQUIRE_THROWS(f.getTextCode(0, "@"));
        REQUIRE_THROWS(f.getWidth(0, "@"));
        REQUIRE(f.getHasDigraphs());
        REQUIRE(f.getFontWidthLocation() == "!somewhere");
        REQUIRE_THROWS(f.getExtraValue("SomeExtra"));
    }
    SECTION("Font with no default width.")
    {
        normalNode.remove(YamlFontSerializer::DEFAULT_WIDTH);
        Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
        std::vector<int> v;
        v.reserve(f.getMaxEncodedValue(0));
        f.getFontWidths(0, std::back_inserter(v));
        REQUIRE(v[74] == 0);
    }
    SECTION("Test commands")
    {
        normalNode[Font::COMMANDS]["EncodingTest"] = 2;
        Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
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
        REQUIRE_THROWS(sz.generateFont(normalNode, "fixedWidth", sable_tests::getTestLocale()));
        normalNode.remove(Font::COMMANDS);
        REQUIRE_THROWS(sz.generateFont(normalNode, "fixedWidth", sable_tests::getTestLocale()));
    }
    SECTION("Test font with fixed widths")
    {
        normalNode.remove(YamlFontSerializer::DEFAULT_WIDTH);
        normalNode[YamlFontSerializer::FIXED_WIDTH] = 8;
        normalNode[Font::ENCODING]["%"] = 4;
        Font f = sz.generateFont(normalNode, "fixedWidth", sable_tests::getTestLocale());
        REQUIRE(std::get<0>(f.getTextCode(0, "%")) == 4);
        REQUIRE(f.getWidth(0, "A") == 8);
        REQUIRE(f.getWidth(0, "A") == f.getWidth(0, "%"));
        std::vector<int> v;
        v.reserve(f.getMaxEncodedValue(0));
        f.getFontWidths(0, std::back_inserter(v));
        REQUIRE(v.front() == v.back());
    }
    SECTION("Fixed width font with default width.")
    {
        normalNode[YamlFontSerializer::FIXED_WIDTH] = "true";
        Font f = sz.generateFont(normalNode, "fixedWidth", sable_tests::getTestLocale());
        REQUIRE(f.getWidth(0, "A") == 8);
        REQUIRE(f.getWidth(0, "A") == f.getWidth(0, "❤"));
    }
    SECTION("Test font without digraphs")
    {
        normalNode[YamlFontSerializer::USE_DIGRAPHS] = "false";
        Font f = sz.generateFont(normalNode, "noDigraphs", sable_tests::getTestLocale());
        REQUIRE(!f.getHasDigraphs());
    }
    SECTION("Test font with extras.")
    {
        normalNode[Font::EXTRAS]["SomeExtra"] = 1;
        Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
        REQUIRE(f.getExtraValue("SomeExtra") == 1);
        REQUIRE_THROWS(f.getExtraValue("SomeMissingExtra"));
    }
    SECTION("Test font with nouns.")
    {
        std::vector<int> data = {0, 1, 2, 3, 4};
        int expectedWidth = 8 * data.size();
        SECTION("Nouns with regular ints")
        {
            normalNode[Font::NOUNS]["SomeNoun"][YamlFontSerializer::CODE_VAL] = data;
            SECTION("Noun with a custom width")
            {
                normalNode[Font::NOUNS]["SomeNoun"][YamlFontSerializer::TEXT_LENGTH_VAL] = 16;
                expectedWidth = 16;
            }
        }
        SECTION("Nouns with hex strings")
        {
            std::vector<std::string> stringData;
            for (auto &i: data) {
                stringData.push_back(std::string("0x") + std::to_string(i));
            }
             normalNode[Font::NOUNS]["SomeNoun"][YamlFontSerializer::CODE_VAL] = stringData;
        }
        Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
        REQUIRE_NOTHROW(f.getNounData(0, "SomeNoun"));
        auto nounData = f.getNounData(0, "SomeNoun");
        REQUIRE(nounData.getWidth() == expectedWidth);
        REQUIRE(data.front() == *nounData);
        int count = 0;
        for (int var: data) {
            int tmp = *(nounData++);
            REQUIRE(tmp == var);
            count++;
        }
        REQUIRE(count == data.size());
    }
    SECTION("Test a font with pages.")
    {
        using sable_tests::EncNode, sable_tests::NounNode;
        std::map<std::string, EncNode> encNode = {
            {"待", {"0x01", "13"}},
            {"祖", {"0x02", "13"}},
            {"老", {"0x03", "", true}},
            {"東", {"0x04", "12"}},
            {"A", {"0x05", "10"}},
        };
        bool usenouns = false;
        int expectedSize = 255 * 2;
        SECTION("Pages without nouns")
        {
            normalNode[YamlFontSerializer::PAGES] = std::vector{encNode};
            Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
        }
        SECTION("Pages with different max encodec characters")
        {
            normalNode[YamlFontSerializer::PAGES].push_back(
                std::map<std::string, decltype(encNode)> {
                    {Font::ENCODING, encNode}
                }
            );
            normalNode[YamlFontSerializer::PAGES][0][YamlFontSerializer::MAX_CHAR] = 5;
            Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
            expectedSize = 255 + 5;
        }
        SECTION("Pages with nouns")
        {
            YAML::Node pageNodeYaml;
            pageNodeYaml[Font::ENCODING] = encNode;
            pageNodeYaml[Font::NOUNS] = std::map<std::string, NounNode>{
                { "マルス", {
                        {"6", "7"},
                        "18"
                }},
            };
            normalNode[YamlFontSerializer::PAGES].push_back(pageNodeYaml);
            usenouns = true;
        }
        normalNode[Font::COMMANDS]["Page0"] = std::map<std::string, std::string>{
            {"code", "0x11"},
            {"page", "0"},
        };
        normalNode[Font::COMMANDS]["Page1"] = std::map<std::string, std::string>{
            {"code", "0x12"},
            {"page", "1"},
        };
        REQUIRE_NOTHROW(sz.generateFont(normalNode, "normal", sable_tests::getTestLocale()));
        Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
        REQUIRE_THROWS(f.getTextCode(2, "A"));
        REQUIRE_THROWS(f.getWidth(2, "A"));
        REQUIRE(std::get<0>(f.getTextCode(0, "A")) == 1);
        REQUIRE(std::get<0>(f.getTextCode(1, "A")) == 5);
        REQUIRE_THROWS(f.getTextCode(0, "待"));
        REQUIRE(std::get<0>(f.getTextCode(1, "待")) == 1);
        REQUIRE(f.getWidth(1, "祖") == 13);
        REQUIRE(f.getWidth(1, "東") == 12);
        REQUIRE(f.getWidth(1, "老") == 8);

        REQUIRE(f.getWidth(0, "A") == 1);
        REQUIRE(f.getWidth(1, "A") == 10);
        REQUIRE_THROWS(f.getWidth(0, "老"));
        if (usenouns) {
            REQUIRE_THROWS(f.getNounData(0, "マルス"));
            REQUIRE_NOTHROW(f.getNounData(1, "マルス"));
            auto it = f.getNounData(1, "マルス");
            REQUIRE(it.getWidth() == 18);
            REQUIRE(*(it++) == 6);
            REQUIRE(*it == 7);
        }
        std::vector<int> widths;
        f.getFontWidths(0, std::back_inserter(widths));
        REQUIRE(widths.size() == f.getMaxEncodedValue(0));
        f.getFontWidths(1, std::back_inserter(widths));
        REQUIRE(widths.size() == (f.getMaxEncodedValue(0) + f.getMaxEncodedValue(1)));
        REQUIRE(widths.size() == expectedSize);
        REQUIRE(widths[255] == 13);
        REQUIRE(widths[257] == 8);
        REQUIRE(widths[258] == 12);

        REQUIRE(f.getCommandData("Page0").page == 0);
        REQUIRE(f.getCommandData("Page1").page == 1);
        REQUIRE(f.getCommandData("NewLine").page == -1);
    }
    SECTION("Testing Unicode normalization.")
    {
        std::string test1 = "\u212B", test2 = "\u0041\u030A", test3 = "\u00C5";
        normalNode[Font::ENCODING]["\u212B"] = 100;
        normalNode[Font::COMMANDS]["zvýraznit"] = 20;
        normalNode[Font::EXTRAS]["östlich"] = 10;
        normalNode[Font::NOUNS]["Åland"][YamlFontSerializer::CODE_VAL] = std::vector{0, 1, 2, 3, 4};
        Font f = sz.generateFont(normalNode, "normal", sable_tests::getTestLocale());
        REQUIRE(std::get<0>(f.getTextCode(0, test1)) == 100);
        REQUIRE(std::get<0>(f.getTextCode(0, test3)) == 100);
        REQUIRE(std::get<0>(f.getTextCode(0, test2)) == 100);
        auto nounTest = "\u0041\u030Aland";
        REQUIRE_NOTHROW(f.getNounData(0, "Åland"));
        REQUIRE_NOTHROW(f.getNounData(0, nounTest));
        REQUIRE_NOTHROW(f.getCommandCode("zvýraznit"));
        REQUIRE_NOTHROW(f.getCommandCode("zv\u0079\u0301raznit"));
        REQUIRE(f.getCommandCode("zvýraznit") == f.getCommandCode("zv\u0079\u0301raznit"));
        REQUIRE_NOTHROW(f.getExtraValue("östlich"));
        REQUIRE_NOTHROW(f.getExtraValue("\u006F\u0308stlich"));
        REQUIRE(f.getExtraValue("östlich") == f.getExtraValue("\u006F\u0308stlich"));
    }
}

TEST_CASE("Test 2-byte fonts.")
{
    using sable::Font, sable::YamlFontSerializer;
    YamlFontSerializer sz;

    std::vector<std::tuple<std::string, int, bool>> commands = {
        {"End", 0xFFFF, false},
        {"NewLine", 0xFFFD, true},
        {"Test", 0xFFFE, true}
    };
    auto menuNode = sable_tests::createSampleNode(true, 2, 0, 8, commands, {}, 0, -1, 0, true);
    SECTION("Test 2-byte font with")
    {
        Font f = sz.generateFont(menuNode, "menu", sable_tests::getTestLocale());
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
    using sable::Font, sable::YamlFontSerializer;
    YamlFontSerializer sz;
    std::vector<std::tuple<std::string, int, bool>> commands = {
        {"End", 0, false},
        {"NewLine", 01, true},
        {"Test", 07, false}
    };
    auto normalNode = sable_tests::createSampleNode(true, 1, 160, 8, commands, {});
    using Catch::Matchers::Contains;
    std::string reqMessage = "Required field \"";
    SECTION("Test Digraph validation.")
    {
        normalNode[YamlFontSerializer::USE_DIGRAPHS] = "invalid";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("must be true or false."));
    }
    SECTION("Check integer validation.")
    {
        normalNode[YamlFontSerializer::DEFAULT_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("must be a scalar integer."));
    }
    SECTION("Check bytewidth validation.")
    {
        normalNode[YamlFontSerializer::BYTE_WIDTH] = 3;
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("must be 1 or 2."));
        normalNode.remove(YamlFontSerializer::BYTE_WIDTH);
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains(reqMessage + YamlFontSerializer::BYTE_WIDTH +"\" is missing."));
    }
    SECTION("Check command value validation.")
    {
        normalNode[YamlFontSerializer::CMD_CHAR] = "invalid";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[YamlFontSerializer::FIXED_WIDTH] = "invalid";
        normalNode.remove(YamlFontSerializer::DEFAULT_WIDTH);
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains(std::string("Field \"") + YamlFontSerializer::FIXED_WIDTH + "\" must be a scalar integer."));
        normalNode[YamlFontSerializer::DEFAULT_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains(std::string("Field \"") + YamlFontSerializer::DEFAULT_WIDTH + "\" must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[YamlFontSerializer::MAX_CHAR] = "invalid";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains(std::string("Field \"") + YamlFontSerializer::MAX_CHAR + "\" must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[YamlFontSerializer::MAX_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains(std::string("Field \"") + YamlFontSerializer::MAX_WIDTH + "\" must be a scalar integer."));
    }
    SECTION("Check font width location validation.")
    {
        normalNode[YamlFontSerializer::FONT_ADDR].push_back(1);
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains(std::string("Field \"") + YamlFontSerializer::FONT_ADDR + "\" must be a string."));
    }
    SECTION("Check Encoding validation.")
    {
        normalNode[Font::ENCODING]["A"][YamlFontSerializer::TEXT_LENGTH_VAL] = "test";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "test", sable_tests::getTestLocale()), Contains("must be an integer."));
        normalNode[Font::ENCODING]["A"][YamlFontSerializer::CODE_VAL] = "test";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "test", sable_tests::getTestLocale()), Contains("must be an integer."));
        normalNode[Font::ENCODING]["A"] = "test";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "test", sable_tests::getTestLocale()), Contains("must be an integer."));
        normalNode[Font::ENCODING].remove("A");
        normalNode[Font::ENCODING]["A"]["test"] = "test";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "test", sable_tests::getTestLocale()), Contains("Field \"Encoding\" has invalid entry \"A\": must define a numeric code."));
        normalNode.remove(Font::ENCODING);
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("is missing."));
        normalNode[Font::ENCODING] = "1";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("must be a map."));
    }
    SECTION("Check Commands validation.")
    {
        normalNode[Font::COMMANDS]["TestBad"] = "Test";
        REQUIRE_THROWS(sz.generateFont(normalNode, "test", sable_tests::getTestLocale()));
        normalNode[Font::COMMANDS].remove("TestBad");
        normalNode[Font::COMMANDS]["NewLine"]["newline"] = YAML::Load("[1, 2, 3]");
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "test", sable_tests::getTestLocale()), Contains("must be a scalar."));
        normalNode[Font::COMMANDS]["NewLine"] = "test";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "test", sable_tests::getTestLocale()), Contains("must be an integer."));
        normalNode.remove(Font::COMMANDS);
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("is missing."));
        normalNode[Font::COMMANDS] = "1";
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("must be a map."));
    }
    SECTION("Check End command is required.")
    {
        normalNode[Font::COMMANDS].remove("End");
        REQUIRE_THROWS_WITH(
            sz.generateFont(normalNode, "test", sable_tests::getTestLocale()),
            Contains("Field \"End\" must be defined in the Commands section.")
        );
    }
    SECTION("Check Newline command is required.")
    {
        normalNode[Font::COMMANDS].remove("NewLine");
        REQUIRE_THROWS_WITH(
            sz.generateFont(normalNode, "test", sable_tests::getTestLocale()),
            Contains("Field \"NewLine\" must be defined in the Commands section.")
        );
    }
    SECTION("Check Extras validation.")
    {
        normalNode[Font::EXTRAS] = "1";
        REQUIRE_THROWS_WITH(
            sz.generateFont(normalNode, "", sable_tests::getTestLocale()),
            Contains("must be a map.")
        );
    }
    SECTION("Check Nouns top level validation")
    {
        SECTION("Invalid node type - scalar.")
        {
            normalNode[Font::NOUNS] = 1;
        }
        SECTION("Invalid node type - sequence.")
        {
            normalNode[Font::NOUNS] = std::vector<int>{1, 2, 3};
        }
        REQUIRE_THROWS_AS(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), sable::FontError);
        REQUIRE_THROWS_WITH(sz.generateFont(normalNode, "", sable_tests::getTestLocale()), Contains("must be a map."));
    }
    SECTION("Nouns map containing only scalars.")
    {
        normalNode[Font::NOUNS][YamlFontSerializer::TEXT_LENGTH_VAL] = "stuff";
        REQUIRE_THROWS_AS(sz.generateFont(normalNode, "bad_nouns", sable_tests::getTestLocale()), sable::FontError);
    }
    SECTION("Nouns code field validation")
    {
        SECTION("Nouns map containing with node missing required fields.")
        {
            normalNode[Font::NOUNS]["Stuff"]["MoreStuff"] = "EvenMoreStuff";
        }
        SECTION("Nouns map containing with node that has invalid code type.")
        {
            normalNode[Font::NOUNS]["Stuff"][YamlFontSerializer::CODE_VAL] = "EvenMoreStuff";
        }
        SECTION("Nouns map containing with node that has invalid code type.")
        {
            normalNode[Font::NOUNS]["Stuff"][YamlFontSerializer::CODE_VAL] = std::vector<std::string>{"a", "b", "c"};
        }
        REQUIRE_THROWS_AS(sz.generateFont(normalNode, "bad_nouns", sable_tests::getTestLocale()), sable::FontError);
        REQUIRE_THROWS_WITH(
            sz.generateFont(normalNode, "bad_nouns", sable_tests::getTestLocale()),
            Contains(std::string("") + "must have a \"" + YamlFontSerializer::CODE_VAL + "\" field that is a sequence of integers.")
        );
    }
    SECTION("Width validation")
    {
        normalNode[Font::NOUNS]["SomeNoun"][YamlFontSerializer::CODE_VAL] = std::vector<int>{0, 1, 2, 3, 4};
        SECTION("Nouns width that is a non-integer scalar.")
        {
            normalNode[Font::NOUNS]["SomeNoun"][YamlFontSerializer::TEXT_LENGTH_VAL] = "something";
        }
        SECTION("Nouns width that is a non-scalar.")
        {
            normalNode[Font::NOUNS]["SomeNoun"][YamlFontSerializer::TEXT_LENGTH_VAL] = std::vector<int>{0, 1, 2, 3, 4};
        }
        REQUIRE_THROWS_AS(sz.generateFont(normalNode, "bad_nouns", sable_tests::getTestLocale()), sable::FontError);
        REQUIRE_THROWS_WITH(
            sz.generateFont(normalNode, "bad_nouns", sable_tests::getTestLocale()),
            Contains(std::string("") + "has a \"" + YamlFontSerializer::TEXT_LENGTH_VAL + "\" field that is not an integer.")
        );
    }
}
