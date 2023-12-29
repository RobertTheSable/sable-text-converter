#include <catch2/catch.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include "parse/unicode.h"

TEST_CASE("Word boundary analysis tests")
{
    std::string line = "This is a basic sentence.";
    BreakIterator l(true, line, icu::Locale::createCanonical("ja_JP"));

    BreakIterator l2(true, line, icu::Locale::createCanonical("en_US"));

    // no idea how else
    l.operator=(l2);

    REQUIRE(l.atStart());
    REQUIRE(*l == "This");
    ++l;
    REQUIRE(*l == " ");
    ++l;
    REQUIRE(*l == "is");
    --l;
    REQUIRE(*l == " ");
    int index = 0;
    std::vector<std::string> expected ={
        " ", "is", " ", "a", " ", "basic", " ", "sentence", "."
    };
    while (!l.done()) {
        if (index >= expected.size()) {
            FAIL(std::string("Unexpected iterator position: \"" + *l +"\"" ));
        }
        REQUIRE(*l == expected[index]);

        ++index;
        ++l;
    }
    REQUIRE(l.front() == "");

    --index;
    --l;
    while (!l.atStart()) {
        if (index < 0) {
            FAIL(std::string("Iterator decremented too much."));
        }
        REQUIRE(*l == expected[index]);
        --l;
        --index;
    }
    ++l;
    REQUIRE(*l == " ");
    auto cl = l;
    REQUIRE(*cl == " ");
    ++l;
    REQUIRE(*cl == " ");
    REQUIRE(*l == "is");
    REQUIRE(l.front() == "i");
}

TEST_CASE("Char boundary with odd glyphs.")
{
    std::string subject = "┌───────────┐";
    BreakIterator l(false, subject, icu::Locale::createCanonical("en_US"));
    REQUIRE(*l == "┌");
}


//TEST_CASE("Boundary with brackets")
//{

//}
