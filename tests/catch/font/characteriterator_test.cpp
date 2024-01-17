#include <catch2/catch.hpp>
#include <chrono>
#include <random>
#include "font/characteriterator.h"

using sable::CharacterIterator;

TEST_CASE("Character Iterator Tests")
{
    std::vector<int> sequence(5, 0);
    std::default_random_engine r(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> ud {0, 10};
    std::generate(sequence.begin(), sequence.end(), [&r, &ud] {
        return ud(r);
    });
    int width = ud(r);
    CharacterIterator it(sequence.begin(), sequence.cend(), width);
    REQUIRE(*it == sequence.front());
    int count = 0;
    for (auto &i : sequence) {
        REQUIRE(*it == i);
        REQUIRE(it++);
        count++;
    }
    REQUIRE(it == false);
    REQUIRE(count == 5);
    REQUIRE(it.getWidth() == width);
}
