#include <catch2/catch.hpp>

#include "textparser_test_helpers.h"

TEST_CASE("Single lines - optional prefixes", "[parser]")
{
    using sable::Font, sable::TextParser;
    auto node = sable_tests::getSampleNode();
    TextParser p(node.as<std::map<std::string, sable::Font>>(), "offset", defLocale, ExportWidth::Off, ExportAddress::On);
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample;
    ByteVector v = {};
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Parse basic string")
    {
        sample.str("ABC");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(res.endOfBlock);
        REQUIRE(res.length == 1 + 2 + 3);
        REQUIRE(res.label.empty());
        REQUIRE(res.metadata == Metadata::No);

        REQUIRE(v.size() == 4);
        REQUIRE(settings.currentAddress == 0x808000);
        REQUIRE(settings.mode == "offset");
        REQUIRE(settings.label.empty());
        REQUIRE((int)v.back() == 0);
    }
    SECTION("Automatic newline insertion")
    {
        sample.str("ABC\n ");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(!res.endOfBlock);
        REQUIRE(res.length == 1 + 2 + 3);
        REQUIRE(res.label.empty());
        REQUIRE(res.metadata == Metadata::No);

        REQUIRE(v.size() == 4);
        REQUIRE((int)v.back() == sable_tests::NEWLINE_VAL);
    }
    SECTION("No newline insertion at end of file")
    {
        sample.str("ABC\n");
        auto res = p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m);
        REQUIRE(res.endOfBlock);
        REQUIRE(res.length == 1 + 2 + 3);
        REQUIRE(res.label.empty());
        REQUIRE(res.metadata == Metadata::No);

        REQUIRE(v.size() == 4);
        REQUIRE((int)v.back() == 0);
    }

    SECTION("Check extras are read correctly.")
    {
        sample.str("[Extra1]");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 0));
        REQUIRE(v.front() == 1);
        REQUIRE(v.size() == 2);
    }
    SECTION("Check commands are read correctly.")
    {
        sample.str("[TestP]");
        REQUIRE(node["offset"][Font::COMMANDS]["TestP"]["code"].as<int>() == 17);
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 0));
        REQUIRE(v == ByteVector({0, 17, 0}));
        REQUIRE(settings.page == 0);
    }
    SECTION("Check commands are read correctly when there is text left in the line.")
    {
        sample.str("[TestP]A");
        REQUIRE(node["normal"][Font::COMMANDS]["Test"]["code"].as<int>() == 7);
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(true, 1));
        REQUIRE(v.size() == 4);
        auto expected = ByteVector({0, 17, 17, 0});
        for (size_t i = 0; i < v.size() ; i++) {
            REQUIRE(expected[i] == v[i]);
        }
        REQUIRE(settings.page == 0);
    }
    SECTION("Check commands with newline settings are read correctly.")
    {
        sample.str("A[NewLine]\n ");
        REQUIRE(p.parseLine(sample, settings, std::back_inserter(v), Metadata::No, m) == std::make_pair(false, 1));
        REQUIRE(v.front() == 17);
        REQUIRE(v.back() == sable_tests::NEWLINE_VAL);
        REQUIRE(v.size() == 2);
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
        REQUIRE(v.front() == 17);
        REQUIRE(v.back() == 2);
        REQUIRE(v.size() == 3);
        REQUIRE(settings.page == 0);
    }
}
