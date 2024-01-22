#include <catch2/catch.hpp>
#include <vector>
#include <cstring>
#include <fstream>
#include <string>
#include <functional>

#include "font/builder.h"
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
    using sable::Font, sable_tests::CommandSample;
    std::vector<CommandSample> commands = {
        {"End", 0, CommandSample::NewLine::No, ""},
        {"NewLine", 01,  CommandSample::NewLine::Yes, "yes"},
        {"Test", 07,  CommandSample::NewLine::No, "True"},
//        {"NoPrefix", 07,  CommandSample::NewLine::No, "false"}
    };
    auto normalNode = sable_tests::createSampleNode(true, 1, 160, 8, commands, {"ll", "la", "e?", "ia", "❤"}, 4);

    SECTION("Test font with widths")
    {
        normalNode[Font::FONT_ADDR] = "!somewhere";
        normalNode[Font::ENCODING]["[Special]"] = 100;
        Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
        REQUIRE(f);
        std::vector<int> v;
        int expectedResult = normalNode[Font::ENCODING]["A"][Font::CODE_VAL].as<int>();
        REQUIRE((bool)f.getTextCode(0, "A"));
        REQUIRE(std::get<0>(f.getTextCode(0, "A").value()) == expectedResult);
        REQUIRE(std::get<0>(f.getTextCode(0, "Special").value()) == 100);
        expectedResult = normalNode[Font::ENCODING]["ll"][Font::CODE_VAL].as<int>();
        REQUIRE(std::get<0>( f.getTextCode(0, "l", "l").value()) == expectedResult);
        REQUIRE(f.getWidth(0, "l") == normalNode[Font::ENCODING]["l"][Font::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(f.getWidth(0, "la") == normalNode[Font::ENCODING]["la"][Font::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(f.getWidth(0, "❤") == normalNode[Font::ENCODING]["❤"][Font::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(!std::get<1>(f.getTextCode(0, "l", "d").value()));
        REQUIRE(f.getMaxEncodedValue(0) == 255);

        v.reserve(f.getMaxEncodedValue(0));
        f.getFontWidths(0, std::back_inserter(v));
        REQUIRE(v.size() == f.getMaxEncodedValue(0));
        REQUIRE(v[0] == normalNode[Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL].as<int>());
        REQUIRE(v[74] == normalNode[Font::DEFAULT_WIDTH].as<int>());

        REQUIRE(f.getCommandValue() == 0);
        REQUIRE(f.getMaxWidth() == 160);
        REQUIRE(f.getByteWidth() == 1);
        REQUIRE(!f.getTextCode(0, "@"));
        REQUIRE_THROWS(f.getWidth(0, "@"));
        REQUIRE(f.getHasDigraphs());
        REQUIRE(f.getFontWidthLocation() == "!somewhere");
        REQUIRE(!f.getExtraValue("SomeExtra"));
    }
    SECTION("Font with no default width.")
    {
        normalNode.remove(Font::DEFAULT_WIDTH);
        Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
        std::vector<int> v;
        v.reserve(f.getMaxEncodedValue(0));
        f.getFontWidths(0, std::back_inserter(v));
        REQUIRE(v[74] == 0);
    }
    SECTION("Test commands")
    {
        normalNode[Font::COMMANDS]["EncodingTest"] = 2;
        Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
        REQUIRE(f.getEndValue() == normalNode[Font::COMMANDS]["End"]["code"].as<int>());
        REQUIRE(f.getCommandData("Test").code == normalNode[Font::COMMANDS]["Test"]["code"].as<int>());
        REQUIRE(f.getCommandData("EncodingTest").code == 2);
        REQUIRE(!f.getCommandData("Test").isNewLine);
        REQUIRE(f.getCommandData("NewLine").isNewLine);
        REQUIRE_THROWS(f.getCommandData("Something"));
    }

#ifndef _MSC_VER
    SECTION("Test deprecated command methods")
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        normalNode[Font::COMMANDS]["EncodingTest"] = 2;
        Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
        REQUIRE(f.getCommandCode("EncodingTest") == 2);
        REQUIRE(f.isCommandNewline("NewLine"));
#pragma GCC diagnostic pop
    }
#endif

    SECTION("Check that End command is required.")
    {
        normalNode[Font::COMMANDS].remove("End");
        REQUIRE_THROWS(sable::FontBuilder::make(normalNode, "fixedWidth", sable_tests::defaultLocale));
        normalNode.remove(Font::COMMANDS);
        REQUIRE_THROWS(sable::FontBuilder::make(normalNode, "fixedWidth", sable_tests::defaultLocale));
    }
    SECTION("Test font with fixed widths")
    {
        normalNode.remove(Font::DEFAULT_WIDTH);
        normalNode[Font::FIXED_WIDTH] = 8;
        normalNode[Font::ENCODING]["%"] = 4;
        Font f = sable::FontBuilder::make(normalNode, "fixedWidth", sable_tests::defaultLocale);
        REQUIRE(f.getTextCode(0, "%"));
        REQUIRE(std::get<0>(f.getTextCode(0, "%").value()) == 4);
        REQUIRE(f.getWidth(0, "A") == 8);
        REQUIRE(f.getWidth(0, "A") == f.getWidth(0, "%"));
        std::vector<int> v;
        v.reserve(f.getMaxEncodedValue(0));
        f.getFontWidths(0, std::back_inserter(v));
        REQUIRE(v.front() == v.back());
    }
    SECTION("Fixed width font with default width.")
    {
        normalNode[Font::FIXED_WIDTH] = "true";
        Font f = sable::FontBuilder::make(normalNode, "fixedWidth", sable_tests::defaultLocale);
        REQUIRE(f.getWidth(0, "A") == 8);
        REQUIRE(f.getWidth(0, "A") == f.getWidth(0, "❤"));
    }
    SECTION("Test font without digraphs")
    {
        normalNode[Font::USE_DIGRAPHS] = "false";
        Font f = sable::FontBuilder::make(normalNode, "noDigraphs", sable_tests::defaultLocale);
        REQUIRE(!f.getHasDigraphs());
    }
    SECTION("Test font with extras.")
    {
        normalNode[Font::EXTRAS]["SomeExtra"] = 1;
        Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
        REQUIRE(f.getExtraValue("SomeExtra") == 1);
        REQUIRE(!f.getExtraValue("SomeMissingExtra"));
    }
    SECTION("Test font with nouns.")
    {
        std::vector<int> data = {0, 1, 2, 3, 4};
        int expectedWidth = 8 * data.size();
        SECTION("Nouns with regular ints")
        {
            normalNode[Font::NOUNS]["SomeNoun"][Font::CODE_VAL] = data;
            SECTION("Noun with a custom width")
            {
                normalNode[Font::NOUNS]["SomeNoun"][Font::TEXT_LENGTH_VAL] = 16;
                expectedWidth = 16;
            }
        }
        SECTION("Nouns with brackets in its name")
        {
            normalNode[Font::NOUNS]["[SomeNoun]"][Font::CODE_VAL] = data;
            normalNode[Font::NOUNS]["[SomeNoun]"][Font::TEXT_LENGTH_VAL] = 16;
            expectedWidth = 16;
        }
        SECTION("Nouns with hex strings")
        {
            std::vector<std::string> stringData;
            for (auto &i: data) {
                stringData.push_back(std::string("0x") + std::to_string(i));
            }
            normalNode[Font::NOUNS]["SomeNoun"][Font::CODE_VAL] = stringData;
        }
        Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
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
            normalNode[Font::PAGES] = std::vector{encNode};
            Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
        }
        SECTION("Pages with different max encodec characters")
        {
            normalNode[Font::PAGES].push_back(
                std::map<std::string, decltype(encNode)> {
                    {Font::ENCODING, encNode}
                }
            );
            normalNode[Font::PAGES][0][Font::MAX_CHAR] = 5;
            Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
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
            normalNode[Font::PAGES].push_back(pageNodeYaml);
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
        REQUIRE_NOTHROW(sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale));
        Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
        // pages out of bound
        REQUIRE_THROWS(f.getTextCode(2, "A"));
        REQUIRE_THROWS(f.getWidth(2, "A"));

        REQUIRE(std::get<0>(f.getTextCode(0, "A").value()) == 1);
        REQUIRE(std::get<0>(f.getTextCode(1, "A").value()) == 5);
        REQUIRE(!f.getTextCode(0, "待"));
        REQUIRE(std::get<0>(f.getTextCode(1, "待").value()) == 1);
        REQUIRE(f.getWidth(1, "祖") == 13);
        REQUIRE(f.getWidth(1, "東") == 12);
        REQUIRE(f.getWidth(1, "老") == 8);

        REQUIRE(f.getWidth(0, "A") == 1);
        REQUIRE(f.getWidth(1, "A") == 10);
        REQUIRE_THROWS(f.getWidth(0, "老"));
        if (usenouns) {
            REQUIRE_THROWS(f.getNounData(0, "マルス"));
            REQUIRE_NOTHROW(f.getNounData(1, "マルス"));
            REQUIRE_THROWS_WITH(f.getNounData(4, "マルス"), "font normal does not have page 4");
            auto it = f.getNounData(1, "マルス");
            REQUIRE(it.getWidth() == 18);
            REQUIRE(*(it++) == 6);
            REQUIRE(*it == 7);
        }
        std::vector<int> widths;
        f.getFontWidths(0, std::back_inserter(widths));
        REQUIRE(widths.size() == f.getMaxEncodedValue(0));
        f.getFontWidths(1, std::back_inserter(widths));
        REQUIRE_THROWS(f.getFontWidths(4, std::back_inserter(widths)));
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
        normalNode[Font::NOUNS]["Åland"][Font::CODE_VAL] = std::vector{0, 1, 2, 3, 4};
        Font f = sable::FontBuilder::make(normalNode, "normal", sable_tests::defaultLocale);
        REQUIRE(std::get<0>(f.getTextCode(0, test1).value()) == 100);
        REQUIRE(std::get<0>(f.getTextCode(0, test3).value()) == 100);
        REQUIRE(std::get<0>(f.getTextCode(0, test2).value()) == 100);
        auto nounTest = "\u0041\u030Aland";
        REQUIRE_NOTHROW(f.getNounData(0, "Åland"));
        REQUIRE_NOTHROW(f.getNounData(0, nounTest));
        REQUIRE_NOTHROW(f.getCommandData("zvýraznit"));
        REQUIRE_NOTHROW(f.getCommandData("zv\u0079\u0301raznit"));
        REQUIRE(f.getCommandData("zvýraznit").code == f.getCommandData("zv\u0079\u0301raznit").code);
        REQUIRE((bool)f.getExtraValue("östlich"));
        REQUIRE((bool)f.getExtraValue("\u006F\u0308stlich"));
        REQUIRE(f.getExtraValue("östlich") == f.getExtraValue("\u006F\u0308stlich"));
    }
}

TEST_CASE("Test 2-byte fonts.")
{
    using sable::Font, sable_tests::CommandSample;

    std::vector<CommandSample> commands = {
        {"End", 0xFFFF, CommandSample::NewLine::No, "No"},
        {"NewLine", 0xFFFD, CommandSample::NewLine::Yes, "no"},
        {"Test", 0xFFFE, CommandSample::NewLine::Yes, "false"}
    };
    auto menuNode = sable_tests::createSampleNode(true, 2, 0, 8, commands, {}, 0, -1, 0, true);
    SECTION("Test 2-byte font with")
    {
        Font f = sable::FontBuilder::make(menuNode, "menu", sable_tests::defaultLocale);
        REQUIRE(f.getByteWidth() == 2);
        REQUIRE(f.getMaxWidth() == 0);
        REQUIRE(f.getFontWidthLocation().empty());
        REQUIRE(!f.getCommandValue());

        REQUIRE(f.getCommandData("Test").isNewLine);

        REQUIRE(!f.getCommandData("Test").isPrefixed);
        REQUIRE(!f.getCommandData("NewLine").isPrefixed);
        REQUIRE(!f.getCommandData("End").isPrefixed);

        REQUIRE(f.getEndValue() == 0xFFFF);
    }
}

TEST_CASE("Test config validation")
{
    using sable::Font, sable_tests::CommandSample;
    std::vector<CommandSample> commands = {
        {"End", 0, CommandSample::NewLine::No, "Yes"},
        {"NewLine", 01, CommandSample::NewLine::Yes, "Yes"},
        {"Test", 07, CommandSample::NewLine::No, "true"}
    };
    auto normalNode = sable_tests::createSampleNode(true, 1, 160, 8, commands, {});
    using Catch::Matchers::Contains;
    std::string reqMessage = "Required field \"";
    SECTION("Test Digraph validation.")
    {
        normalNode[Font::USE_DIGRAPHS] = "invalid";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("must be true or false."));
    }
    SECTION("Check integer validation.")
    {
        normalNode[Font::DEFAULT_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("must be a scalar integer."));
    }
    SECTION("Check bytewidth validation.")
    {
        normalNode[Font::BYTE_WIDTH] = 3;
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("must be 1 or 2."));
        normalNode.remove(Font::BYTE_WIDTH);
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains(reqMessage + Font::BYTE_WIDTH +"\" is missing."));
    }
    SECTION("Check command value validation.")
    {
        normalNode[Font::CMD_CHAR] = "invalid";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[Font::FIXED_WIDTH] = "invalid";
        normalNode.remove(Font::DEFAULT_WIDTH);
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains(std::string("Field \"") + Font::FIXED_WIDTH + "\" must be a scalar integer."));
        normalNode[Font::DEFAULT_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains(std::string("Field \"") + Font::DEFAULT_WIDTH + "\" must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[Font::MAX_CHAR] = "invalid";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains(std::string("Field \"") + Font::MAX_CHAR + "\" must be a scalar integer."));
    }
    SECTION("Check fixed width validation.")
    {
        normalNode[Font::MAX_WIDTH] = "invalid";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains(std::string("Field \"") + Font::MAX_WIDTH + "\" must be a scalar integer."));
    }
    SECTION("Check font width location validation.")
    {
        normalNode[Font::FONT_ADDR].push_back(1);
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains(std::string("Field \"") + Font::FONT_ADDR + "\" must be a string."));
    }
    SECTION("Check Encoding validation.")
    {
        normalNode[Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL] = "test";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale), Contains("\"A\": has a \"length\" field that is not an integer."));
        normalNode[Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL] = 0;
        normalNode[Font::ENCODING]["A"][Font::CODE_VAL] = "test";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale), Contains("\"A\": has a \"code\" field that is not an integer."));
        normalNode[Font::ENCODING]["A"][Font::CODE_VAL] = 0;
        REQUIRE_THROWS_WITH(
            sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
            Contains("Field \"Encoding\" has invalid entry \"A\": has a \"code\" field that matches the command value, which is not allowed.")
        );
        normalNode[Font::ENCODING]["A"] = "test";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale), Contains("\"A\": has a \"code\" field that is not an integer."));
        normalNode[Font::ENCODING].remove("A");
        normalNode[Font::ENCODING]["A"]["test"] = "test";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale), Contains("Field \"Encoding\" has invalid entry \"A\": must define a numeric code."));
        normalNode.remove(Font::ENCODING);
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("is missing."));
        normalNode[Font::ENCODING] = "1";
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("must be a map."));
    }
    SECTION("Check Commands validation.")
    {
        SECTION("Code validation")
        {
            normalNode[Font::COMMANDS]["TestBad"] = "Test";
            REQUIRE_THROWS(sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale));
            YAML::Node n;
            n[Font::CODE_VAL] = "Test";
            normalNode[Font::COMMANDS]["TestBad"] = n;
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                Contains("Field \"code\" must be an integer.")
            );
            normalNode[Font::COMMANDS]["TestBad"] = std::array{1,2,3};
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                Contains("has invalid entry \"TestBad\": must define a numeric code.")
            );
        }

        SECTION("Prefix validation")
        {
            YAML::Node n;
            n[Font::CODE_VAL] = 1;
            SECTION("Invalid scalar")
            {
                n[Font::CMD_PREFIX] = "something";
                normalNode[Font::COMMANDS]["TestBad"] = n;
                REQUIRE_THROWS_WITH(
                    sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                    Contains("Field \"prefix\" must be yes, true, no, or false.")
                );
            }
            SECTION("Invalid type")
            {
                n[Font::CMD_PREFIX] = YAML::Load("[1, 2, 3]");
                normalNode[Font::COMMANDS]["TestBad"] = n;
                REQUIRE_THROWS_WITH(
                    sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                    Contains("Field \"prefix\" must be a scalar.")
                );
            }
        }

        SECTION("Newline validation")
        {
            normalNode[Font::COMMANDS]["NewLine"]["newline"] = YAML::Load("[1, 2, 3]");
            REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale), Contains("Field \"newline\" must be a scalar."));
        }

        SECTION("Overall node validation")
        {
            normalNode.remove(Font::COMMANDS);
            REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("is missing."));
            normalNode[Font::COMMANDS] = "1";
            REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("must be a map."));
        }
    }

    SECTION("Check Pages validation.")
    {
        YAML::Node textNode;
        SECTION("Invalid node type")
        {
            textNode["A"] = std::vector{"something"};
            normalNode[Font::PAGES] = std::vector{textNode};
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                Contains("Field \"Pages\" has invalid entry \"A\"")
            );
        }
        SECTION("Invalid node value")
        {
            textNode["A"] = "something";
            normalNode[Font::PAGES] = std::vector{textNode};
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                Contains("Field \"Pages #1\" has invalid entry \"A\"")
            );
        }
        SECTION("Invalid node value")
        {
            textNode["A"] = 0;
            normalNode[Font::PAGES] = std::vector{textNode};
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                Contains("Field \"Pages #1\" has invalid entry \"A\": has a \"code\" field that matches the command value, which is not allowed.")

            );

        }
        SECTION("Invalid Pages node")
        {
            normalNode[Font::PAGES] = std::vector{"A", "B"};
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                Contains("Field \"Pages\" must be a sequence of maps.")
            );
        }
        SECTION("Invalid Pages node")
        {
            normalNode[Font::PAGES] = "A";
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
                Contains("Field \"Pages\" must be a sequence of maps.")
            );
        }
    }

    SECTION("Check End command is required.")
    {
        normalNode[Font::COMMANDS].remove("End");
        try {
            sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale);
            FAIL("no error thrown");
        } catch (sable::FontError &e) {
            REQUIRE(e.getField() == "End");
            REQUIRE(e.getName() == "test");
            REQUIRE(e.getMessage() == "defined in the Commands section.");
            REQUIRE_THAT(e.what(), Contains("Field \"End\" must be defined in the Commands section."));
        } catch (std::runtime_error &e) {
            FAIL("unhandled errpr");
        }
    }
    SECTION("Check Newline command is required.")
    {
        normalNode[Font::COMMANDS].remove("NewLine");
        REQUIRE_THROWS_WITH(
            sable::FontBuilder::make(normalNode, "test", sable_tests::defaultLocale),
            Contains("Field \"NewLine\" must be defined in the Commands section.")
        );
    }
    SECTION("Check Extras validation.")
    {
        SECTION("Bad Extras node")
        {
            normalNode[Font::EXTRAS] = "1";
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale),
                Contains("must be a map.")
            );
        }
        SECTION("Bad Extras entry")
        {
            normalNode[Font::EXTRAS]["argle"] = "blargle";
            REQUIRE_THROWS_WITH(
                sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale),
                Contains("Field \"Extras\" has invalid entry \"argle\": must define a numeric code.")
            );
        }
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
        REQUIRE_THROWS_AS(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), sable::FontError);
        REQUIRE_THROWS_WITH(sable::FontBuilder::make(normalNode, "", sable_tests::defaultLocale), Contains("must be a map."));
    }
    SECTION("Nouns map containing only scalars.")
    {
        normalNode[Font::NOUNS][Font::TEXT_LENGTH_VAL] = "stuff";
        REQUIRE_THROWS_AS(sable::FontBuilder::make(normalNode, "bad_nouns", sable_tests::defaultLocale), sable::FontError);
    }
    SECTION("Nouns code field validation")
    {
        SECTION("Nouns map containing with node missing required fields.")
        {
            normalNode[Font::NOUNS]["Stuff"]["MoreStuff"] = "EvenMoreStuff";
        }
        SECTION("Nouns map containing with node that has invalid code type.")
        {
            normalNode[Font::NOUNS]["Stuff"][Font::CODE_VAL] = "EvenMoreStuff";
        }
        SECTION("Nouns map containing with node that has invalid code type.")
        {
            normalNode[Font::NOUNS]["Stuff"][Font::CODE_VAL] = std::vector<std::string>{"a", "b", "c"};
        }
        REQUIRE_THROWS_AS(sable::FontBuilder::make(normalNode, "bad_nouns", sable_tests::defaultLocale), sable::FontError);
        REQUIRE_THROWS_WITH(
            sable::FontBuilder::make(normalNode, "bad_nouns", sable_tests::defaultLocale),
            Contains(std::string("") + "must have a \"" + Font::CODE_VAL + "\" field that is a sequence of integers.")
        );
    }
    SECTION("Width validation")
    {
        normalNode[Font::NOUNS]["SomeNoun"][Font::CODE_VAL] = std::vector<int>{0, 1, 2, 3, 4};
        SECTION("Nouns width that is a non-integer scalar.")
        {
            normalNode[Font::NOUNS]["SomeNoun"][Font::TEXT_LENGTH_VAL] = "something";
        }
        SECTION("Nouns width that is a non-scalar.")
        {
            normalNode[Font::NOUNS]["SomeNoun"][Font::TEXT_LENGTH_VAL] = std::vector<int>{0, 1, 2, 3, 4};
        }
        REQUIRE_THROWS_AS(sable::FontBuilder::make(normalNode, "bad_nouns", sable_tests::defaultLocale), sable::FontError);
        REQUIRE_THROWS_WITH(
            sable::FontBuilder::make(normalNode, "bad_nouns", sable_tests::defaultLocale),
            Contains(std::string("") + "has a \"" + Font::TEXT_LENGTH_VAL + "\" field that is not an integer.")
        );
    }
}
