#include <catch2/catch.hpp>

#include "font/normalize.h"

TEST_CASE("Test normalization")
{
    std::string test1 = "\u212B", test2 = "\u0041\u030A", test3 = "\u00C5";

    REQUIRE(sable::normalize("something") == "something");
    REQUIRE(sable::normalize(test1) == sable::normalize(test2));
    REQUIRE(sable::normalize(test3) == sable::normalize(test2));
    REQUIRE(sable::normalize("Åland") == sable::normalize("\u0041\u030Aland"));
    REQUIRE(sable::normalize("zvýraznit") == sable::normalize("zv\u0079\u0301raznit"));
    REQUIRE(sable::normalize("östlich") == sable::normalize("\u006F\u0308stlich"));
}
