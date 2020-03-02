#include <catch2/catch.hpp>
#include <vector>
#include "font.h"

YAML::Node getSampleFontsNode()
{
    static YAML::Node sampleNode = YAML::LoadFile("sample_text_map.yml");
    return sampleNode;
}

TEST_CASE("Test font with widths")
{
    using sable::Font;
    auto node = getSampleFontsNode()["normal"];
    Font f(node, "normal");
    std::vector<int> v;
    v.reserve(f.getMaxEncodedValue());
    f.getFontWidths(std::back_inserter(v));
    REQUIRE(v.size() == f.getMaxEncodedValue());
    REQUIRE(v[0] == 6);
    REQUIRE(v[0x4c] == 0);
}

TEST_CASE("Test font with fixed widths")
{
    using sable::Font;
    auto node = getSampleFontsNode()["opening"];
    Font f(node, "opening");
    std::vector<int> v;
    v.reserve(f.getMaxEncodedValue());
    f.getFontWidths(std::back_inserter(v));
    REQUIRE(v.size() == f.getMaxEncodedValue());
    bool allWidthsMatch = true;
    int firstWidth = f.getWidth("A");
    for (auto &currentWidth: v) {
        allWidthsMatch &= currentWidth == firstWidth;
        if (!allWidthsMatch) {
            REQUIRE(currentWidth == firstWidth);
        }
    }
    REQUIRE(allWidthsMatch);
}

TEST_CASE("Test font with fixed widths using alias")
{
    using sable::Font;
    auto node = getSampleFontsNode()["fixedWidth"];
    Font f(node, "fixedWidth");
    std::vector<int> v;
    v.reserve(f.getMaxEncodedValue());
    f.getFontWidths(std::back_inserter(v));
    REQUIRE(v.size() == f.getMaxEncodedValue());
    bool allWidthsMatch = true;
    int firstWidth = f.getWidth("A");
    for (auto &currentWidth: v) {
        allWidthsMatch &= currentWidth == firstWidth;
        if (!allWidthsMatch) {
            REQUIRE(currentWidth == firstWidth);
        }
    }
    REQUIRE(allWidthsMatch);
}
