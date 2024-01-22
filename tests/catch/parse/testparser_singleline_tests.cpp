#include <catch2/catch.hpp>

#include "textparser_test_helpers.h"

TEST_CASE("Single lines", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    node["menu"][sable::Font::ENCODING]["[SpecialEnd]"] = 0xFFFF;
    TextParser p(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Parse basic string")
    {
        sample.str("ABC");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(res.endOfBlock);
        REQUIRE(res.length == 6);
        REQUIRE(res.label.empty());
        REQUIRE(res.metadata == Metadata::No);

        REQUIRE(v.size() == 5);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.mode == "normal");
        REQUIRE(settings.label.empty());
        REQUIRE((int)v.back() == 0);
    }
    SECTION("CR stripping")
    {
        sample.str("ABC\r");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(res.endOfBlock);
        REQUIRE(res.length == 6);
        REQUIRE(res.label.empty());
        REQUIRE(res.metadata == Metadata::No);

        REQUIRE(v.size() == 5);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.mode == "normal");
        REQUIRE(settings.label.empty());
        REQUIRE((int)v.back() == 0);
    }
    SECTION("Automatic newline insertion")
    {
        sample.str("ABC\n ");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 6);
        REQUIRE(res.label.empty());
        REQUIRE(res.metadata == Metadata::No);

        REQUIRE(v.size() == 5);
        REQUIRE((int)v.back() == sable_tests::NEWLINE_VAL);
    }
    SECTION("No newline insertion at end of file")
    {
        sample.str("ABC\n");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(res.endOfBlock);
        REQUIRE(res.length == 6);
        REQUIRE(res.label.empty());
        REQUIRE(res.metadata == Metadata::No);

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
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m).length == expected);
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
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m).length == expected);
        REQUIRE(v.size() == 10);
        REQUIRE(v == ByteVector({0x4A, 53, 0x4B, 53, 0x26, 30, 53, 0x4C, 0, 0}));
    }
    SECTION("Parse line with digraphs with font without digraphs")
    {
        settings.mode = "nodigraph";
        sample.str("ll la ld e?");
        auto encNode = node["normal"][Font::ENCODING];
        int expected = sample.str().length() * 8;
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m).length == expected);
        REQUIRE(v.size() == 13);
        REQUIRE(v == ByteVector({0x26, 0x26, 53, 0x26, 0x1b, 53, 0x26, 0x1e, 53, 0x1f, 59, 0, 0}));
    }
    SECTION("Check digraphs including multibyte punctuation")
    {
        sample.str("e† A");
        sable::TextParser::Result result;
        auto expected =
                node["normal"][Font::ENCODING]["e†"][Font::TEXT_LENGTH_VAL].as<int>() +
                node["normal"][Font::ENCODING][" "][Font::TEXT_LENGTH_VAL].as<int>() +
                node["normal"][Font::ENCODING]["A"][Font::TEXT_LENGTH_VAL].as<int>();
        REQUIRE_NOTHROW(result = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m));
        REQUIRE(result.length == expected);
        REQUIRE(v.size() == 5);
    }
    SECTION("Check digraphs starting with a space")
    {
        sample.str("al h");
        node["normal"][Font::ENCODING] = std::map<std::string, int>{
            { " h", 1},
            { "al", 2}
        };
        TextParser pAlt(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
        sable::TextParser::Result result;
        REQUIRE_NOTHROW(result = pAlt.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m));
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
        TextParser pAlt(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
        sable::TextParser::Result result;
        REQUIRE_NOTHROW(result = pAlt.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m));
        REQUIRE(v.front() == 2);
        REQUIRE(v[1] == 1);
        REQUIRE(v.size() == 4);
    }
    SECTION("Check that a Noun is recognized")
    {
        node["normal"][Font::NOUNS]["Noun"][Font::CODE_VAL] = std::vector<int>{1,1,1};
        TextParser p2(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
        sample.str("Noun");
        sable::TextParser::Result result;
        REQUIRE_NOTHROW(result = p2.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m));
        REQUIRE(v.size() == 5);
        REQUIRE(v.front() == 1);
        REQUIRE(v[1] == 1);
        REQUIRE(v[2] == 1);
    }
    SECTION("Check extras are read correctly.")
    {
        sample.str("[Extra1]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 0));
        REQUIRE(v.front() == 1);
        REQUIRE(v.size() == 3);
    }
    SECTION("Check commands are read correctly.")
    {
        sample.str("[Test]");
        REQUIRE(node["normal"][Font::COMMANDS]["Test"]["code"].as<int>() == 7);
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 0));
        REQUIRE(v == ByteVector({0, 7, 0, 0}));
        REQUIRE(settings.page == 0);
    }
    SECTION("Check commands are read correctly when there is text left in the line.")
    {
        sample.str("[Test]A");
        REQUIRE(node["normal"][Font::COMMANDS]["Test"]["code"].as<int>() == 7);
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 1));
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
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(false, 1));
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == sable_tests::NEWLINE_VAL);
        REQUIRE(v.size() == 3);
        REQUIRE(settings.page == 0);
    }
    SECTION("Check command newline setting is preserved after reading an extra or hex value.")
    {
        SECTION("hex")
        {
            sample.str("A[NewLine][02]\n ");
        }
        SECTION("extra")
        {
            sample.str("A[NewLine][Extra2]\n ");
        }

        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(false, 1));
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 2);
        REQUIRE(v.size() == 4);
        REQUIRE(settings.page == 0);
    }
    SECTION("Check bracketed text")
    {
        sample.str("[special]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 6));
        REQUIRE(v.front() == 0x4D);
        REQUIRE(v.size() == 3);
    }
    SECTION("Check binary insertion.")
    {
        sample.str("[010A03]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 0));
        REQUIRE(v == ByteVector({3, 10, 1, 0, 0}));
    }
    SECTION("Manual end with autoend")
    {
        sample.str("ABC[End]XYZ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 6));
        REQUIRE(v.size() == 5);
        REQUIRE(v.back() == 0);
        REQUIRE(settings.page == 0);
    }
    SECTION("Manual end without autoend")
    {
        settings.autoend = ParseSettings::Autoend::Off;
        sample.str("ABC[End]XYZ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 17));
        REQUIRE(v.size() == 8);
        REQUIRE(v.back() == 26);
        REQUIRE(settings.page == 0);
    }
    SECTION("Manual end without command code")
    {
        settings.mode = "menu";

        SECTION("Manual end with autoend")
        {
            sample.str("ABC[End]XYZ");
            auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
            REQUIRE(res.endOfBlock);
            REQUIRE(res.length == 8*3);
            REQUIRE(v.size() == 8);
            REQUIRE(v.back() == 0xFF);
            REQUIRE(settings.page == 0);
        }
        SECTION("Manual end without autoend")
        {
            settings.autoend = ParseSettings::Autoend::Off;
            sample.str("ABC[End]XYZ");
            auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
            REQUIRE(res.endOfBlock);
            REQUIRE(res.length == 8*6);
            REQUIRE(v.size() == 14);
            REQUIRE(v.back() == 00);
            REQUIRE(v[v.size()-2] == 25);
            REQUIRE(settings.page == 0);
        }
        SECTION("Manual end via hex with autoend")
        {
            sample.str("ABC[FFFF]XYZ");
            auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
            REQUIRE(res.endOfBlock);
            REQUIRE(res.length == 8*3);
            REQUIRE(v.size() == 8);
            REQUIRE(v.back() == 0xFF);
            REQUIRE(settings.page == 0);
        }
        SECTION("Manual end via hex without autoend")
        {
            settings.autoend = ParseSettings::Autoend::Off;
            sample.str("ABC[FFFF]XYZ");
            auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
            REQUIRE(res.endOfBlock);
            REQUIRE(res.length == 8*6);
            REQUIRE(v.size() == 14);
            REQUIRE(v.back() == 00);
            REQUIRE(v[v.size()-2] == 25);
            REQUIRE(settings.page == 0);
        }
        SECTION("Manual end via end alias with autoend")
        {
            sample.str("ABC[SpecialEnd]XYZ");
            auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
            REQUIRE(res.endOfBlock);
            REQUIRE(res.length == 8*4);
            REQUIRE(v.size() == 8);
            REQUIRE(v.back() == 0xFF);
            REQUIRE(settings.page == 0);
        }
        SECTION("Manual end via end alias without autoend")
        {
            settings.autoend = ParseSettings::Autoend::Off;
            sample.str("ABC[SpecialEnd]XYZ");
            auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
            REQUIRE(res.endOfBlock);
            REQUIRE(res.length == 8*7);
            REQUIRE(v.size() == 14);
            REQUIRE(v.back() == 00);
            REQUIRE(v[v.size()-2] == 25);
            REQUIRE(settings.page == 0);
        }
    }

    SECTION("empty string - no setting")
    {
        SECTION("empty")
        {
            sample.str("");
        }
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 0));
        REQUIRE(v.size() == 0);
    }
    SECTION("empty string - after setting")
    {
        SECTION("empty")
        {
            sample.str("");
        }
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::Yes, m) == std::make_pair(true, 0));
        REQUIRE(v.size() == 2);
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

        TextParser pwp(node.as<std::map<std::string, sable::Font>>(), "normal", jpLocale, ExportWidth::Off, ExportAddress::On);
        SECTION("Switch from page 0 > 1")
        {
            sample.str("ABC[Page1]シーダ");
            REQUIRE_NOTHROW(pwp.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m));
            REQUIRE(settings.page == 1);
            REQUIRE(v.size() == 10);
            REQUIRE(v[5] == 0x5c);
        }
        SECTION("Switch from page 0 > 1 > 0")
        {
            sample.str("A[Page1]シ[Page0]C");
            REQUIRE_NOTHROW(pwp.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m));
            REQUIRE(settings.page == 0);
            REQUIRE(v.size() == 9);
            REQUIRE(v[0] == 1);
            REQUIRE(v[3] == 0x5c);
            REQUIRE(v[6] == 3);
        }
    }
}
