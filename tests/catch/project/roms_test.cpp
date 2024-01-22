#include <catch2/catch.hpp>

#include <array>

#include "project/builder.h"

TEST_CASE("Valid ROM Decoding")
{
    YAML::Node n;
    n["name"] = "something";
    n["file"] = "something.sfc";
    sable::ProjectSerializer::Rom rom;

    SECTION("No includes")
    {
        SECTION("Automatic headers")
        {
            SECTION("Header setting not specified")
            {
                rom = n.as<sable::ProjectSerializer::Rom>();
            }
            SECTION("Header setting = auto")
            {
                n["header"] = "auto";
                rom = n.as<sable::ProjectSerializer::Rom>();
            }
            REQUIRE(rom.hasHeader == 0);
        }
        SECTION("No header")
        {
            n["header"] = "false";
            rom = n.as<sable::ProjectSerializer::Rom>();
            REQUIRE(rom.hasHeader == -1);
        }
        SECTION("With header")
        {
            n["header"] = "true";
            rom = n.as<sable::ProjectSerializer::Rom>();
            REQUIRE(rom.hasHeader == 1);
        }
        REQUIRE(rom.includes.empty());
    }

    SECTION("With includes")
    {
        n["includes"] = std::array{"include.asm", "include2.asm", "folder/include.bin"};
        rom = n.as<sable::ProjectSerializer::Rom>();
        REQUIRE(rom.hasHeader == 0);
        REQUIRE(rom.includes.size() == 3);
        REQUIRE(rom.includes[0] == "include.asm");
        REQUIRE(rom.includes[1] == "include2.asm");
        REQUIRE(rom.includes[2] == "folder/include.bin");
    }

    REQUIRE(rom.file == "something.sfc");
    REQUIRE(rom.name == "something");
}

TEST_CASE("Invalid ROM Decoding")
{
    YAML::Node n;
    n["name"] = "something";
    n["file"] = "something.sfc";
    n["header"] = "bad!";
    REQUIRE_THROWS(n.as<sable::ProjectSerializer::Rom>());
}
