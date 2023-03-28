#include <catch2/catch.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "parse.h"
#include "helpers.h"
#include <iostream>

typedef std::vector<unsigned char> ByteVector;

namespace {
    const auto jpLocale = "ja_JP.UTF-8";
    const auto defLocale = "en_US.UTF-8";
}

TEST_CASE("Class properties", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<sable::FontList>(), "normal", "en_US.utf-8");
    SECTION("Iterate over fonts.")
    {
        auto& fonts = p.getFonts();
        REQUIRE(fonts.size() == 3);
        REQUIRE(fonts.contains("normal"));
        REQUIRE(fonts.contains("menu"));
        REQUIRE(fonts.contains("nodigraph"));
        REQUIRE(!fonts.contains("test"));
    }
}

TEST_CASE("Single lines", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<sable::FontList>(), "normal", defLocale);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Parse basic string")
    {
        sample.str("ABC");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 6));
        REQUIRE(v.size() == 5);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.mode == "normal");
        REQUIRE(settings.label.empty());
        REQUIRE((int)v.back() == 0);
    }
    SECTION("Automatic newline insertion")
    {
        sample.str("ABC\n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(false, 6));
        REQUIRE(v.size() == 5);
        REQUIRE((int)v.back() == 1);
    }
    SECTION("No newline insertion at end of file")
    {
        sample.str("ABC\n");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 6));
        REQUIRE(v.size() == 5);
        REQUIRE((int)v.back() == 0);
    }
    SECTION("Parse strings with UTF8")
    {
        sample.str("test ❤");
        auto encNode = node["normal"][Font::ENCODING];
        int expected = (encNode["t"][Font::TEXT_LENGTH_VAL].as<int>() * 2) +
                encNode["e"][Font::TEXT_LENGTH_VAL].as<int>() +
                encNode["s"][Font::TEXT_LENGTH_VAL].as<int>() +
                encNode[" "][Font::TEXT_LENGTH_VAL].as<int>() +
                encNode["❤"][Font::TEXT_LENGTH_VAL].as<int>();
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m).second == expected);
        REQUIRE(v.size() == 8);
    }
    SECTION("Parse line with supported digraphs")
    {
        sample.str("ll la ld e?");
        auto&& encNode = node["normal"][Font::ENCODING];
        int expected = (encNode[" "][Font::TEXT_LENGTH_VAL].as<int>() * 3) +
                encNode["e?"][Font::TEXT_LENGTH_VAL].as<int>() +
                encNode["ll"][Font::TEXT_LENGTH_VAL].as<int>() +
                encNode["la"][Font::TEXT_LENGTH_VAL].as<int>() +
                encNode["l"][Font::TEXT_LENGTH_VAL].as<int>() +
                encNode["d"][Font::TEXT_LENGTH_VAL].as<int>();
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m).second == expected);
        REQUIRE(v.size() == 10);
        REQUIRE(v == ByteVector({0x4A, 53, 0x4B, 53, 0x26, 30, 53, 0x4C, 0, 0}));
    }
    SECTION("Parse line with digraphs with font without digraphs")
    {
        settings.mode = "nodigraph";
        sample.str("ll la ld e?");
        auto encNode = node["normal"][Font::ENCODING];
        int expected = sample.str().length() * 8;
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m).second == expected);
        REQUIRE(v.size() == 13);
        REQUIRE(v == ByteVector({0x26, 0x26, 53, 0x26, 0x1b, 53, 0x26, 0x1e, 53, 0x1f, 59, 0, 0}));
    }
    SECTION("Check digraphs including multibyte punctuation")
    {
        sample.str("e† A");
        std::pair<bool, int> result;
        auto expected =
                node["normal"][Font::ENCODING]["e†"][Font::TEXT_LENGTH_VAL].as<int>() +
                node["normal"][Font::ENCODING][" "][Font::TEXT_LENGTH_VAL].as<int>() +
                node["normal"][Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL].as<int>();
        REQUIRE_NOTHROW(result = p.parseLine(sample, settings, std::back_inserter(v), m));
        REQUIRE(result.second == expected);
        REQUIRE(v.size() == 5);
    }
    SECTION("Check digraphs starting with a space")
    {
        sample.str("al h");
        node["normal"][Font::ENCODING] = std::map<std::string, int>{
            { " h", 1},
            { "al", 2}
        };
        TextParser pAlt(node.as<sable::FontList>(), "normal", defLocale);
        std::pair<bool, int> result;
        REQUIRE_NOTHROW(result = pAlt.parseLine(sample, settings, std::back_inserter(v), m));
        REQUIRE(v.front() == 2);
        REQUIRE(v[1] == 1);
        REQUIRE(v.size() == 4);
    }
    SECTION("Check digraphs starting and ending with a space")
    {
        sample.str("a  h");
        node["normal"][Font::ENCODING] = std::map<std::string, int>{
            { " h", 1},
            { "a ", 2}
        };
        TextParser pAlt(node.as<sable::FontList>(), "normal", defLocale);
        std::pair<bool, int> result;
        REQUIRE_NOTHROW(result = pAlt.parseLine(sample, settings, std::back_inserter(v), m));
        REQUIRE(v.front() == 2);
        REQUIRE(v[1] == 1);
        REQUIRE(v.size() == 4);
    }
    SECTION("Check that a Noun is recognized")
    {
        node["normal"][Font::NOUNS]["Noun"][Font::CODE_VAL] = std::vector<int>{1,1,1};
        TextParser p2(node.as<sable::FontList>(), "normal", defLocale);
        sample.str("Noun");
        std::pair<bool, int> result;
        REQUIRE_NOTHROW(result = p2.parseLine(sample, settings, std::back_inserter(v), m));
        REQUIRE(v.size() == 5);
        REQUIRE(v.front() == 1);
        REQUIRE(v[1] == 1);
        REQUIRE(v[2] == 1);
    }
    SECTION("Check extras are read correctly.")
    {
        sample.str("[Extra1]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(v.front() == 1);
        REQUIRE(v.size() == 3);
    }
    SECTION("Check commands are read correctly.")
    {
        sample.str("[Test]");
        REQUIRE(node["normal"][Font::COMMANDS]["Test"]["code"].as<int>() == 7);
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(v == ByteVector({0, 7, 0, 0}));
        REQUIRE(settings.page == 0);
    }
    SECTION("Check commands are read correctly when there is text left in the line.")
    {
        sample.str("[Test]A");
        REQUIRE(node["normal"][Font::COMMANDS]["Test"]["code"].as<int>() == 7);
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 1));
        REQUIRE(v.size() == 5);
        auto expected = ByteVector({0, 7, 1, 0, 0});
        for (size_t i = 0; i < v.size() ; i++) {
            REQUIRE(expected[i] == v[i]);
        }
        REQUIRE(settings.page == 0);
    }
    SECTION("Check commands with newline settings are read correctly.")
    {
        sample.str("A[NewLine]\n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(false, 1));
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 1);
        REQUIRE(v.size() == 3);
        REQUIRE(settings.page == 0);
    }
    SECTION("Check bracketed text")
    {
        sample.str("[special]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 6));
        REQUIRE(v.front() == 0x4D);
        REQUIRE(v.size() == 3);
    }
    SECTION("Check binary insertion.")
    {
        sample.str("[010A03]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(v == ByteVector({3, 10, 1, 0, 0}));
    }
    SECTION("Manual end with autoend")
    {
        sample.str("ABC[End]XYZ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 6));
        REQUIRE(v.size() == 5);
        REQUIRE(v.back() == 0);
        REQUIRE(settings.page == 0);
    }
    SECTION("Manual end without autoend")
    {
        settings.autoend = false;
        sample.str("ABC[End]XYZ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m).second == 17);
        REQUIRE(v.size() == 8);
        REQUIRE(v.back() == 26);
        REQUIRE(settings.page == 0);
    }
    SECTION("Commands that switch pages.")
    {
        using sable_tests::EncNode, sable_tests::NounNode;
        YAML::Node pageNodeYaml2;
        pageNodeYaml2[Font::ENCODING] = std::map<std::string, EncNode>{
            {"シ", {"0x5c", "11"}},
            {"ー", {"0xb9", "10"}},
            {"ダ", {"0x88", "11"}},
            {"E", {"0x3f", "10"}},
        };
        pageNodeYaml2[Font::NOUNS] = std::map<std::string, NounNode>{
            { "マルス", {
                    {"6", "7"},
                    "18"
            }},
        };
        node["normal"][Font::COMMANDS]["Page0"] = std::map<std::string, std::string>{
            {"code", "0x11"},
            {"page", "0"},
        };
        node["normal"][Font::COMMANDS]["Page1"] = std::map<std::string, std::string>{
            {"code", "0x12"},
            {"page", "1"},
        };
        node["normal"][Font::PAGES].push_back(pageNodeYaml2);

        TextParser pwp(node.as<sable::FontList>(), "normal", jpLocale);
        SECTION("Switch from page 0 > 1")
        {
            sample.str("ABC[Page1]シーダ");
            REQUIRE_NOTHROW(pwp.parseLine(sample, settings, std::back_inserter(v), m));
            REQUIRE(settings.page == 1);
            REQUIRE(v.size() == 10);
            REQUIRE(v[5] == 0x5c);
        }
        SECTION("Switch from page 0 > 1 > 0")
        {
            sample.str("A[Page1]シ[Page0]C");
            REQUIRE_NOTHROW(pwp.parseLine(sample, settings, std::back_inserter(v), m));
            REQUIRE(settings.page == 0);
            REQUIRE(v.size() == 9);
            REQUIRE(v[0] == 1);
            REQUIRE(v[3] == 0x5c);
            REQUIRE(v[6] == 3);
        }
    }
}

TEST_CASE("Test ending behavior", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<sable::FontList>(), "normal", defLocale);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Parser indicated there are more lines in stream.")
    {
        sample.str("ABC\n"
                   "test");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(false, 6));
        REQUIRE(v.size() == 5);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.mode == "normal");
        REQUIRE(settings.label.empty());
        REQUIRE((int)v.back() == 01);
    }
}

TEST_CASE("Default settings", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    SECTION("Normal Settings")
    {
        TextParser p(node.as<sable::FontList>(), "normal", defLocale);
        auto settings = p.getDefaultSetting(0x808000);
        REQUIRE(settings.mode == "normal");
        REQUIRE(settings.label.empty());
        REQUIRE(settings.maxWidth == 160);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(!settings.printpc);
        REQUIRE(settings.autoend);
    }
    SECTION("Menu Settings")
    {
        TextParser p(node.as<sable::FontList>(), "menu", defLocale);
        auto settings = p.getDefaultSetting(0x908000);
        REQUIRE(settings.mode == "menu");
        REQUIRE(settings.label.empty());
        REQUIRE(settings.maxWidth == 0);
        REQUIRE(settings.currentAddress == 0x908000);
        REQUIRE(!settings.printpc);
        REQUIRE(settings.autoend);
    }
}

TEST_CASE("Change parser settings", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<sable::FontList>(), "normal", defLocale);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Set address")
    {
        settings.currentAddress = 0;
        sample.str("@address $808000 \n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(false, 0));
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.autoend);
        REQUIRE(settings.label.empty());
        REQUIRE(v.empty());
    }
    SECTION("Automatic address setting")
    {
        sample.str("@address auto \n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(false, 0));
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Set label")
    {
        sample.str("@label test01 ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.label == "test01");
        REQUIRE((!v.empty() && v.front() == 0));
    }
    SECTION("Set text mode")
    {
        sample.str("@type menu \n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(false, 0));
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.label.empty());
        REQUIRE(settings.mode == "menu");
        REQUIRE(v.size() == 0);
        sample.clear();
        sample.str("ABD");
        auto result = p.parseLine(sample, settings, std::back_inserter(v), m);
        REQUIRE(result.second == 24);
        REQUIRE(v.size() == 8);
        REQUIRE(v == ByteVector({0,0,1,0,3,0,0xff,0xff}));
    }
    SECTION("Set text mode to default")
    {
        settings.mode = "menu";
        sample.str("@type default");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(settings.mode == "normal");
    }

    SECTION("Disable autoend")
    {
        sample.str("@autoend off");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(v.empty());
        REQUIRE(!settings.autoend);
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Enable autoend")
    {
        settings.autoend = false;
        sample.str("@autoend on");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(!v.empty());
        REQUIRE(settings.autoend);
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Turn on print pc")
    {
        sample.str("@printpc ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(settings.printpc);
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Set page correctly")
    {
        using sable_tests::EncNode, sable_tests::NounNode;
        YAML::Node pageNodeYaml2;
        pageNodeYaml2[Font::ENCODING] = std::map<std::string, EncNode>{
            {"シ", {"0x5c", "11"}},
        };
        node["normal"][Font::PAGES].push_back(pageNodeYaml2);
        TextParser pwp(node.as<sable::FontList>(), "normal", jpLocale);
        sample.str("@page 1");
        REQUIRE(pwp.parseLine(sample, settings, std::back_inserter(v), m) == std::make_pair(true, 0));
        REQUIRE(settings.page == 1);
    }
}

TEST_CASE("Multiline scenarios", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<sable::FontList>(), "normal", defLocale);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Switching between modes")
    {
        sample.str("ABC\n"
                   "@type menu\n"
                   "ABC\n");
        while (!p.parseLine(sample, settings, std::back_inserter(v), m).first) {}
        REQUIRE(v.size() == 11);
        REQUIRE(v == ByteVector({1, 2, 3, 0, 0, 1, 0, 2, 0, 0xFF, 0xFF}));
    }
}

TEST_CASE("Parser error checking", "[parser]")
{
    using sable::Font, sable::TextParser, Catch::Matchers::Contains;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<sable::FontList>(), "normal", defLocale);
    auto settings = p.getDefaultSetting(0x80800);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Don't parse if address isn't set.")
    {
        sample.str("Test");
        settings.currentAddress = 0;
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Attempted to parse text before address was set.");
    }
    SECTION("Undefined text.")
    {
        sample.str("%");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), Contains("not found in Encoding of font normal"));
    }
    SECTION("Undefined bracketed value")
    {
        sample.str("[Missing]");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), Contains("not found in font normal"));
    }
    SECTION("Unenclosed bracket.")
    {
        sample.str("[Missing");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Closing bracket not found.");
    }
    SECTION("Unsupported setting")
    {
        sample.str("@missing");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Unrecognized option \"missing\"");
    }
    SECTION("Setting with missing required option")
    {
        sample.str("@label ");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Option \"label\" is missing a required value");
    }
    SECTION("Width validation")
    {
        sample.str("@width b");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Invalid option \"b\" for width: must be off or a decimal number.");
    }
    SECTION("Autoend validation")
    {
        sample.str("@autoend b");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Invalid option \"b\" for autoend: must be on or off.");
    }
    SECTION("Address validation")
    {
        sample.str("@address t");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Invalid option \"t\" for address: must be auto or a SNES address.");
        sample.clear();
        sample.str("@address 000000");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Invalid option \"000000\" for address: must be auto or a SNES address.");
    }
    SECTION("Address is too large.")
    {
        m = sable::util::Mapper(sable::util::MapperType::EXLOROM, false, true, 0x600000);
        sample.str("@address $608000");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), m), "Invalid option \"$608000\" for address: address is too large for the specified ROM size.");
        sample.clear();
    }
    SECTION("Page out of range in font with no extra pages.")
    {
        sample.str("@page 1");
        REQUIRE_THROWS_WITH(
            p.parseLine(sample, settings, std::back_inserter(v), m),
            "Page 1 not found in font normal"
        );
    }
    SECTION("Page out of range in font with one extra pages.")
    {
        using sable_tests::EncNode, sable_tests::NounNode;
        YAML::Node pageNodeYaml2;
        pageNodeYaml2[Font::ENCODING] = std::map<std::string, EncNode>{
            {"シ", {"0x5c", "11"}},
        };
        node["normal"][Font::PAGES].push_back(pageNodeYaml2);
        node["normal"][Font::COMMANDS]["Page2"] = std::map<std::string, std::string>{
            {"code", "0x12"},
            {"page", "2"},
        };
        SECTION("With implicit page switching.")
        {
            sample.str("@page 2");
        }
        SECTION("With a command that switches pages.")
        {
            sample.str("[Page2]");
        }
        TextParser pwp(node.as<sable::FontList>(), "normal", jpLocale);
        REQUIRE_THROWS_WITH(
            pwp.parseLine(sample, settings, std::back_inserter(v), m),
            "Page 2 not found in font normal"
        );
    }
}
