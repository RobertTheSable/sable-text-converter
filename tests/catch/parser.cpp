#include <catch2/catch.hpp>
#include <sstream>
#include <functional>
#include <iostream>
#include "parse.h"

typedef std::vector<unsigned char> ByteVector;

YAML::Node getSampleNode()
{
    static YAML::Node sampleNode = YAML::LoadFile("sample_text_map.yml");
    return sampleNode;
}

TEST_CASE("Parse One line", "[parser]")
{
    sable::TextParser p(getSampleNode(), "normal");
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample("This is a test.");
    ByteVector v = {};
    auto result = p.parseLine(sample, settings, std::back_inserter(v));
    REQUIRE(result.second == 71);
    REQUIRE(result.first == true);
    REQUIRE(v.size() == 17);
    REQUIRE(v.front() == 0x14);
    REQUIRE(v.back() == 0);
    std::string test((char*)&v.front(), v.size());
    REQUIRE(std::hash<std::string>{}(test) == 0xd83ae5514d524d5a);
}

TEST_CASE("Parse Another Line", "[parser]")
{
    sable::TextParser p(getSampleNode(), "normal");
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample("This is a different test.");
    ByteVector v = {};
    auto result = p.parseLine(sample, settings, std::back_inserter(v));
    std::string test((char*)&v.front(), v.size());
    REQUIRE(result.first == true);
    REQUIRE(std::hash<std::string>{}(test) == 0x506cd23bdfb8f2d5);
}

TEST_CASE("Reading End will end the parsing", "[parser]")
{
    sable::TextParser p(getSampleNode(), "normal");
    auto settings = p.getDefaultSetting(0x808000);
    std::istringstream sample("This string end early.[End]\n"
                              "This is fluff.\n");
    ByteVector v = {};
    auto result = p.parseLine(sample, settings, std::back_inserter(v));
    std::string test((char*)&v.front(), v.size());
    REQUIRE(result.first == true);
}

TEST_CASE("Parse Several Lines With Settings", "[parser]")
{
    sable::TextParser p(getSampleNode(), "nodigraph");
    std::istringstream sample("@address $e0e9b0\n"
                       "@width 160\n"
                       "@label dialogue_07\n"
                       "[_88][04][ShowPortait][6a][04]If you're wounded, you can \n"
                       "rest in the forts, where you'll \n"
                       "slowly recover vitality. [WaitForA][CloseFrame][02]\n"
                       "# length: 102\n"
                       );
    auto settings = p.getDefaultSetting(0);
    ByteVector v = {};
    p.parseLine(sample, settings, std::back_inserter(v));
    p.parseLine(sample, settings, std::back_inserter(v));
    auto result = p.parseLine(sample, settings, std::back_inserter(v));
    REQUIRE(result.first == false);
    REQUIRE(result.second == 0);
    REQUIRE(v.size() == 0);
    REQUIRE(settings.currentAddress == 0xe0e9b0);
    REQUIRE(settings.label == "dialogue_07");
    REQUIRE(settings.maxWidth == 160);
    result = p.parseLine(sample, settings, std::back_inserter(v));
    REQUIRE(result.first == false);
    REQUIRE(result.second == 139);
    p.parseLine(sample, settings, std::back_inserter(v));
    p.parseLine(sample, settings, std::back_inserter(v));
    result = p.parseLine(sample, settings, std::back_inserter(v));
    REQUIRE(result.first == true);
    std::string test((char*)&v.front(), v.size());
    REQUIRE(std::hash<std::string>{}(test) == 0x7741b2e19aeedb5b);
    REQUIRE(v.front() == 0);
    REQUIRE(v.back() == 0);
    REQUIRE(v.size() == 102);
}
