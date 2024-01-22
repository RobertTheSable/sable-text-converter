#include <catch2/catch.hpp>
#include <tuple>
#include <array>
#include "data/textblockrange.h"

using Block = sable::TextBlockRange;

TEST_CASE("Block Comparison")
{
    REQUIRE(Block{0, 10} == Block{0, 10});
    REQUIRE(Block{0, 10} < Block{1, 10});
    REQUIRE(Block{0, 9} < Block{0, 10});
}

using sable::Blocks;

TEST_CASE("Block Collisions")
{
    Blocks blockList;
    REQUIRE(blockList.addBlock(0, 10u, "1", "file") == Blocks::Collision::None);
    REQUIRE(blockList.addBlock(30, 40u, "2", "file") == Blocks::Collision::None);
    REQUIRE(blockList.addBlock(15, 25u, "3", "file") == Blocks::Collision::None);
    // need to test the != operator
    // catch compains if I use it without the extra parens
    REQUIRE((blockList.addBlock(0, 10u) != Blocks::Collision::None));
    REQUIRE((Blocks::Collision::None != blockList.addBlock(0, 10u)));
    struct { //NOLINT
        int start;
        std::size_t end;
        Blocks::Collision expectedCollide;
        std::string expectedLabel;
    } cases[] = {
        {0, 10u, Blocks::Collision::Same, "1"},
        {0, 9u, Blocks::Collision::Next, "1"},
        {1, 11u, Blocks::Collision::Prior, "1"},
        {27, 35u, Blocks::Collision::Next, "2"},
        {20, 26u, Blocks::Collision::Prior, "3"},
    };
    for (auto tCase: cases) {
        auto result = blockList.addBlock(tCase.start, tCase.end, "sample", "file");
        REQUIRE(result == tCase.expectedCollide);
        REQUIRE(result->label == tCase.expectedLabel);
    }
}
