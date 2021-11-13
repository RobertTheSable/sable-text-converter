#include <catch2/catch.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "parse.h"
#include "helpers.h"
#include <iostream>

typedef std::vector<unsigned char> ByteVector;

TEST_CASE("Class properties", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node, "normal", "en_US.UTF-8");
    SECTION("Iterate over fonts.")
    {
        auto& fonts = p.getFonts();
        REQUIRE(fonts.size() == 3);
        REQUIRE(fonts.find("normal") != fonts.end());
        REQUIRE(fonts.find("menu") != fonts.end());
        REQUIRE(fonts.find("nodigraph") != fonts.end());
        REQUIRE(fonts.find("test") == fonts.end());
    }
}

TEST_CASE("Single lines", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node, "normal", "en_US.UTF-8");
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    SECTION("Parse basic string")
    {
        sample.str("ABC");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 6));
        REQUIRE(v.size() == 5);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.mode == "normal");
        REQUIRE(settings.label.empty());
        REQUIRE((int)v.back() == 0);
    }
    SECTION("Automatic newline insertion")
    {
        sample.str("ABC\n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(false, 6));
        REQUIRE(v.size() == 5);
        REQUIRE((int)v.back() == 1);
    }
    SECTION("No newline insertion at end of file")
    {
        sample.str("ABC\n");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 6));
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
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)).second == expected);
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
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)).second == expected);
        REQUIRE(v.size() == 10);
        REQUIRE(v == ByteVector({0x4A, 53, 0x4B, 53, 0x26, 30, 53, 0x4C, 0, 0}));
    }
    SECTION("Parse line with digraphs with font without digraphs")
    {
        settings.mode = "nodigraph";
        sample.str("ll la ld e?");
        auto encNode = node["normal"][Font::ENCODING];
        int expected = sample.str().length() * 8;
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)).second == expected);
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
        REQUIRE_NOTHROW(result = p.parseLine(sample, settings, std::back_inserter(v)));
        REQUIRE(result.second == expected);
        REQUIRE(v.size() == 5);
    }
    SECTION("Check that a Noun is recognized")
    {
        node["normal"][Font::NOUNS]["Noun"][Font::CODE_VAL] = std::vector<int>{1,1,1};
        p = TextParser(node, "normal", "en_US.UTF-8");
        sample.str("Noun");
        std::pair<bool, int> result;
        REQUIRE_NOTHROW(result = p.parseLine(sample, settings, std::back_inserter(v)));
        REQUIRE(v.size() == 5);
        REQUIRE(v.front() == 1);
        REQUIRE(v[1] == 1);
        REQUIRE(v[2] == 1);
    }
    SECTION("Check extras are read correctly.")
    {
        sample.str("[Extra1]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 0));
        REQUIRE(v.front() == 1);
        REQUIRE(v.size() == 3);
    }
    SECTION("Check commands are read correctly.")
    {
        sample.str("[Test]");
        REQUIRE(node["normal"][Font::COMMANDS]["Test"]["code"].as<int>() == 7);
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 0));
        REQUIRE(v == ByteVector({0, 7, 0, 0}));
    }
    SECTION("Check commands are read correctly when there is text left in the line.")
    {
        sample.str("[Test]A");
        REQUIRE(node["normal"][Font::COMMANDS]["Test"]["code"].as<int>() == 7);
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 1));
        REQUIRE(v.size() == 5);
        auto expected = ByteVector({0, 7, 1, 0, 0});
        for (size_t i = 0; i < v.size() ; i++) {
            REQUIRE(expected[i] == v[i]);
        }
    }
    SECTION("Check commands with newline settings are read correctly.")
    {
        sample.str("A[NewLine]\n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(false, 1));
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 1);
        REQUIRE(v.size() == 3);
    }
    SECTION("Check bracketed text")
    {
        sample.str("[special]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 6));
        REQUIRE(v.front() == 0x4D);
        REQUIRE(v.size() == 3);
    }
    SECTION("Check binary insertion.")
    {
        sample.str("[010A03]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 0));
        REQUIRE(v == ByteVector({3, 10, 1, 0, 0}));
    }
    SECTION("Manual end with autoend")
    {
        sample.str("ABC[End]XYZ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 6));
        REQUIRE(v.size() == 5);
        REQUIRE(v.back() == 0);
    }
    SECTION("Manual end without autoend")
    {
        settings.autoend = false;
        sample.str("ABC[End]XYZ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)).second == 17);
        REQUIRE(v.size() == 8);
        REQUIRE(v.back() == 26);
    }
}

TEST_CASE("Test ending behavior", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node, "normal", "en_US.UTF-8");
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    SECTION("Parser indicated there are more lines in stream.")
    {
        sample.str("ABC\n"
                   "test");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(false, 6));
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
        TextParser p(node, "normal", "en_US.UTF-8");
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
        TextParser p(node, "menu", "en_US.UTF-8");
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
    TextParser p(node, "normal", "en_US.UTF-8");
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    SECTION("Set address")
    {
        settings.currentAddress = 0;
        sample.str("@address $808000 \n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(false, 0));
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.autoend);
        REQUIRE(settings.label.empty());
        REQUIRE(v.empty());
    }
    SECTION("Automatic address setting")
    {
        sample.str("@address auto \n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(false, 0));
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Set label")
    {
        sample.str("@label test01 ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 0));
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.label == "test01");
        REQUIRE((!v.empty() && v.front() == 0));
    }
    SECTION("Set text mode")
    {
        sample.str("@type menu \n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(false, 0));
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.label.empty());
        REQUIRE(settings.mode == "menu");
        REQUIRE(v.size() == 0);
        sample.clear();
        sample.str("ABD");
        auto result = p.parseLine(sample, settings, std::back_inserter(v));
        REQUIRE(result.second == 24);
        REQUIRE(v.size() == 8);
        REQUIRE(v == ByteVector({0,0,1,0,3,0,0xff,0xff}));
    }
    SECTION("Set text mode to default")
    {
        settings.mode = "menu";
        sample.str("@type default");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 0));
        REQUIRE(settings.mode == "normal");
    }

    SECTION("Disable autoend")
    {
        sample.str("@autoend off");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 0));
        REQUIRE(v.empty());
        REQUIRE(!settings.autoend);
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Enable autoend")
    {
        settings.autoend = false;
        sample.str("@autoend on");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 0));
        REQUIRE(!v.empty());
        REQUIRE(settings.autoend);
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Turn on print pc")
    {
        sample.str("@printpc ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v)) == std::make_pair(true, 0));
        REQUIRE(settings.printpc);
        REQUIRE(settings.currentAddress == 0x808000);
    }
}

TEST_CASE("Multiline scenarios", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node, "normal", "en_US.UTF-8");
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    SECTION("Switching between modes")
    {
        sample.str("ABC\n"
                   "@type menu\n"
                   "ABC\n");
        while (!p.parseLine(sample, settings, std::back_inserter(v)).first) {}
        REQUIRE(v.size() == 11);
        REQUIRE(v == ByteVector({1, 2, 3, 0, 0, 1, 0, 2, 0, 0xFF, 0xFF}));
    }
}

TEST_CASE("Parser error checking", "[parser]")
{
    using sable::Font, sable::TextParser, Catch::Matchers::Contains;
    auto node = sable_tests::getSampleNode();
    TextParser p(node, "normal", "en_US.UTF-8");
    auto settings = p.getDefaultSetting(0x80800);
    std::istringstream sample;
    ByteVector v = {};
    SECTION("Don't parse if address isn't set.")
    {
        sample.str("Test");
        settings.currentAddress = 0;
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), "Attempted to parse text before address was set.");
    }
    SECTION("Undefined text.")
    {
        sample.str("%");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), Contains("not found in Encoding of font normal"));
    }
    SECTION("Undefined bracketed value")
    {
        sample.str("[Missing]");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), Contains("not found in font normal"));
    }
    SECTION("Unenclosed bracket.")
    {
        sample.str("[Missing");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), "Closing bracket not found.");
    }
    SECTION("Unsupported setting")
    {
        sample.str("@missing");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), "Unrecognized option \"missing\"");
    }
    SECTION("Setting with missing required option")
    {
        sample.str("@label ");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), "Option \"label\" is missing a required value");
    }
    SECTION("Width validation")
    {
        sample.str("@width b");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), "Invalid option \"b\" for width: must be off or a decimal number.");
    }
    SECTION("Autoend validation")
    {
        sample.str("@autoend b");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), "Invalid option \"b\" for autoend: must be on or off.");
    }
    SECTION("Address validation")
    {
        sample.str("@address t");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), "Invalid option \"t\" for address: must be auto or a SNES address.");
        sample.clear();
        sample.str("@address 000000");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v)), "Invalid option \"000000\" for address: must be auto or a SNES address.");
    }
}
