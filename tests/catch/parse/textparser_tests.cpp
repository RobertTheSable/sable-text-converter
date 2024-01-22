#include <catch2/catch.hpp>
#include <sstream>
#include <iostream>

#include "textparser_test_helpers.h"

TEST_CASE("Class properties", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<std::map<std::string, sable::Font>>(), "normal", "en_US.utf-8", ExportWidth::Off, ExportAddress::On);
    SECTION("Iterate over fonts.")
    {
        auto& fonts = p.getFonts();
        REQUIRE(fonts.size() == 4);
        REQUIRE(sable::contains(fonts, "normal"));
        REQUIRE(sable::contains(fonts, "menu"));
        REQUIRE(sable::contains(fonts, "nodigraph"));
        REQUIRE(sable::contains(fonts, "offset"));
        REQUIRE(!sable::contains(fonts, "test"));
    }
}

TEST_CASE("Test ending behavior", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Parser indicated there are more lines in stream.")
    {
        sample.str("ABC\n"
                   "test");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(false, 6));
        REQUIRE(v.size() == 5);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.mode == "normal");
        REQUIRE(settings.label.empty());
        REQUIRE((int)v.back() == sable_tests::NEWLINE_VAL);
    }
}

TEST_CASE("Default settings", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    SECTION("Normal Settings")
    {
        TextParser p(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
        auto settings = p.getDefaultSetting(0x808000);
        REQUIRE(settings.mode == "normal");
        REQUIRE(settings.label.empty());
        REQUIRE(settings.maxWidth == 160);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(!settings.printpc);
        REQUIRE(settings.autoend == ParseSettings::Autoend::On);
        REQUIRE(settings.exportAddress == ExportAddress::On);
        REQUIRE(settings.exportWidth == ExportWidth::Off);
        REQUIRE(settings.endOnLabel == ParseSettings::EndOnLabel::Off);
    }
    SECTION("Menu Settings")
    {
        TextParser p(node.as<std::map<std::string, sable::Font>>(), "menu", defLocale, ExportWidth::Off, ExportAddress::On);
        auto settings = p.getDefaultSetting(0x908000);
        REQUIRE(settings.mode == "menu");
        REQUIRE(settings.label.empty());
        REQUIRE(settings.maxWidth == 0);
        REQUIRE(settings.currentAddress == 0x908000);
        REQUIRE(!settings.printpc);
        REQUIRE(settings.autoend == ParseSettings::Autoend::On);
        REQUIRE(settings.exportAddress == ExportAddress::On);
        REQUIRE(settings.exportWidth == ExportWidth::Off);
        REQUIRE(settings.endOnLabel == ParseSettings::EndOnLabel::Off);
    }
}

TEST_CASE("Change parser settings", "[parser]")
{

    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Set address")
    {
        settings.currentAddress = 0;
        sample.str("@address $808000 \n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(false, 0));
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.autoend == ParseSettings::Autoend::On);
        REQUIRE(settings.label.empty());
        REQUIRE(v.empty());
    }
    SECTION("Automatic address setting")
    {
        sample.str("@address auto \n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(false, 0));
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Set label")
    {
        sample.str("@label test01 ");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.label == "test01");
        REQUIRE(v.empty());
    }
    SECTION("Set text mode")
    {
        sample.str("@type menu \n ");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.label.empty());
        REQUIRE(settings.mode == "menu");
        REQUIRE(v.size() == 0);
        sample.clear();
        sample.str("ABD");
        auto result = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(result.length == 24);
        REQUIRE(v.size() == 8);
        REQUIRE(v == ByteVector({0,0,1,0,3,0,0xff,0xff}));
    }
    SECTION("Set text mode to default")
    {
        settings.mode = "menu";
        sample.str("@type default");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);
        REQUIRE(settings.mode == "normal");
    }

    SECTION("Disable autoend")
    {
        sample.str("@autoend off");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(v.empty());
        REQUIRE(settings.autoend == ParseSettings::Autoend::Off);
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Enable autoend")
    {
        settings.autoend = ParseSettings::Autoend::Off;
        sample.str("@autoend on");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(v.empty());
        REQUIRE(settings.autoend == ParseSettings::Autoend::On);
        REQUIRE(settings.currentAddress == 0x808000);
    }
    SECTION("Turn on print pc")
    {
        sample.str("@printpc ");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

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
        TextParser pwp(node.as<std::map<std::string, sable::Font>>(), "normal", jpLocale, ExportWidth::Off, ExportAddress::On);
        sample.str("@page 1");
        auto res = pwp.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);
        REQUIRE(settings.page == 1);
    }
    SECTION("Set Width")
    {
        sample.str("@width 100");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);
        REQUIRE(settings.maxWidth == 100);
    }
    SECTION("Disable width checking")
    {
        sample.str("@width off");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.maxWidth == -1);
    }

    SECTION("Disable address exporting")
    {
        sample.str("@export_address off");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.exportAddress == ExportAddress::Off);
    }
    SECTION("Enable address exporting")
    {
        sample.str("@export_address on");
        settings.exportAddress = ExportAddress::Off;
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.exportAddress == ExportAddress::On);
    }

    SECTION("Disable width exporting")
    {
        sample.str("@export_width off");
        settings.exportWidth = ExportWidth::On;
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.exportWidth == ExportWidth::Off);
    }
    SECTION("Enable width exporting")
    {
        sample.str("@export_width on");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.exportWidth == ExportWidth::On);
    }

    SECTION("Disable ending on label")
    {
        sample.str("@end_on_label off");
        settings.endOnLabel = ParseSettings::EndOnLabel::On;
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.endOnLabel == ParseSettings::EndOnLabel::Off);
    }
    SECTION("Enable ending on label")
    {
        sample.str("@end_on_label on");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.metadata == Metadata::Yes);

        REQUIRE(settings.endOnLabel == ParseSettings::EndOnLabel::On);
    }
}

TEST_CASE("Multiline scenarios", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Switching between modes")
    {
        sample.str("ABC\n"
                   "@type menu\n"
                   "ABC\n");
        while (!p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m).endOfBlock) {}
        REQUIRE(v.size() == 11);
        REQUIRE(v == ByteVector({1, 2, 3, 0, 0, 1, 0, 2, 0, 0xFF, 0xFF}));
    }
    SECTION("Changing labels with end_on_label enabled")
    {
        REQUIRE(v.size() == 0);
        sample.str("@end_on_label on\n"
                   "@label something\n"
                   "ABC\n"
                   "@label other");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(false, 0));
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 0));
        REQUIRE(v.size() == 0);
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);

        REQUIRE(v.size() == 5);
        REQUIRE(res.endOfBlock);
        REQUIRE(res.length == 0);
        REQUIRE(res.label == "something");
    }
}

TEST_CASE("Parser error checking", "[parser]")
{
    using sable::Font, sable::TextParser, Catch::Matchers::Contains;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<std::map<std::string, sable::Font>>(), "normal", defLocale, ExportWidth::Off, ExportAddress::On);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Don't parse if address isn't set.")
    {
        sample.str("Test");
        settings.currentAddress = 0;
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Attempted to parse text before address was set.");
    }
    SECTION("Undefined text.")
    {
        sample.str("%");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), Contains("not found in Encoding of font normal"));
    }
    SECTION("Undefined bracketed value")
    {
        sample.str("[Missing]");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), Contains("not found in font normal"));
    }
    SECTION("Unenclosed bracket.")
    {
        sample.str("[Missing");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Closing bracket not found.");
    }
    SECTION("Unsupported setting")
    {
        sample.str("@missing");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Unrecognized option \"missing\"");
    }
    SECTION("Setting with missing required option")
    {
        sample.str("@label ");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Option \"label\" is missing a required value");
    }
    SECTION("Missing font.")
    {
        sample.str("@type doesnotexist");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Font \"doesnotexist\" was not defined");
    }
    SECTION("Width validation")
    {
        sample.str("@width b");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Invalid option \"b\" for width: must be off or a decimal number.");
    }
    SECTION("Autoend validation")
    {
        sample.str("@autoend b");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Invalid option \"b\" for autoend: must be on or off.");
    }
    SECTION("Address validation")
    {
        sample.str("@address t");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Invalid option \"t\" for address: must be auto or a SNES address.");
        sample.clear();
        sample.str("@address 000000");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Invalid option \"000000\" for address: must be auto or a SNES address.");
    }
    SECTION("Address is too large.")
    {
        m = sable::util::Mapper(sable::util::MapperType::EXLOROM, false, true, 0x600000);
        sample.str("@address $608000");
        REQUIRE_THROWS_WITH(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m), "Invalid option \"$608000\" for address: address is too large for the specified ROM size.");
        sample.clear();
    }
    SECTION("Page number is not a number.")
    {
        sample.str("@page abcd");
        REQUIRE_THROWS_WITH(
            p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m),
            "Page \"abcd\" is not a decimal integer."
        );
    }
    SECTION("Page out of range in font with no extra pages.")
    {
        sample.str("@page 1");
        REQUIRE_THROWS_WITH(
            p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m),
            "Page 1 not found in font normal"
        );
    }
    SECTION("Page number is not a number.")
    {
        sample.str("@");
        REQUIRE_THROWS_WITH(
            p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m),
            "@ symbol found, but no setting specified."
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
        TextParser pwp(node.as<std::map<std::string, sable::Font>>(), "normal", jpLocale, ExportWidth::Off, ExportAddress::On);
        REQUIRE_THROWS_WITH(
            pwp.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m),
            "Page 2 not found in font normal"
        );
    }
}
