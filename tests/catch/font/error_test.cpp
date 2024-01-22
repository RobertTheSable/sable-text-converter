#include <catch2/catch.hpp>

#include "font/error.h"

TEST_CASE("Font error with no field or message")
{
    sable::FontError fe(1, "test", "", "");
    REQUIRE(std::string(fe.what()) == "In font \"test\", line 1: Unknown error.");
}
