#include <catch2/catch.hpp>

#include "project/mapperconv.h"

TEST_CASE("Mapper YAML decoding")
{
    using sable::util::MapperType;
    YAML::Node n;
    SECTION("lorom")
    {
        n["type"] = "lorom";
        REQUIRE(n["type"].as<MapperType>() == MapperType::LOROM);
    }
    SECTION("hirom")
    {
        n["type"] = "hirom";
        REQUIRE(n["type"].as<MapperType>() == MapperType::HIROM);
    }
    SECTION("exlorom")
    {
        n["type"] = "exlorom";
        REQUIRE(n["type"].as<MapperType>() == MapperType::EXLOROM);
    }
    SECTION("exhirom")
    {
        n["type"] = "exhirom";
        REQUIRE(n["type"].as<MapperType>() == MapperType::EXHIROM);
    }
    SECTION("invalid")
    {
        n["type"] = "other";
        REQUIRE_THROWS(n["type"].as<MapperType>() == MapperType::INVALID);
    }
}
