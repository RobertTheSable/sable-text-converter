#include <catch2/catch.hpp>

#include "parse/block.h"

using sable::Block;

TEST_CASE("Block with no split")
{
    Block subject(0x808000, 0x908000, std::vector<unsigned char>(0x100, 0u));
    REQUIRE(subject.getNextAddress() == 0x808100);
    REQUIRE(subject.length() == 0x100);
    REQUIRE(!subject.bankSplit());
    REQUIRE(subject.bankBounds.size() == 1);
    REQUIRE(subject.bankBounds.back().fileSuffix == "");
    REQUIRE(subject.bankBounds.back().labelPrefix == "");


    REQUIRE(subject.bankBounds.back().address == 0x808000);
    REQUIRE(subject.bankBounds.back().start == 0);
    REQUIRE(subject.bankBounds.back().length == 0x100);
}


TEST_CASE("Block with one split")
{
    Block subject(0x80FF81, 0x908000, std::vector<unsigned char>(0x100, 0u));
    REQUIRE(subject.getNextAddress() == 0x908081);
    REQUIRE(subject.length() == 0x100);
    REQUIRE(subject.bankSplit());
    REQUIRE(subject.bankBounds.size() == 2);

    REQUIRE(subject.bankBounds.front().fileSuffix == "bank");
    REQUIRE(subject.bankBounds.front().labelPrefix == "$");
    REQUIRE(subject.bankBounds.front().address == 0x908000);
    REQUIRE(subject.bankBounds.front().start == 0x7F);
    REQUIRE(subject.bankBounds.front().length == 0x81);

    REQUIRE(subject.bankBounds.back().address == 0x80FF81);
    REQUIRE(subject.bankBounds.back().start == 0);
    REQUIRE(subject.bankBounds.back().length == 0x7F);
}

TEST_CASE("Block which crosses two banks")
{
    REQUIRE_THROWS_WITH(
        Block (0x80FF81, 0x908000, std::vector<unsigned char>(0x10000, 0u)),
        "read data that would cross two banks, aborting."
    );
}
