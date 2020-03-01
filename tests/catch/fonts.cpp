#include <catch2/catch.hpp>
#include <vector>
#include "font.h"

YAML::Node getSampleNode()
{
    static YAML::Node sampleNode = YAML::LoadFile("sample_text_map.yml");
    return sampleNode;
}

TEST_CASE("Test font with widths")
{
    using sable::Font;
    auto node = getSampleNode()["normal"];
    Font f(node, "normal");
    std::vector<int> v;
    v.reserve(f.getMaxEncodedValue());
    f.getFontWidths(std::back_inserter(v));
    REQUIRE(v.size() == f.getMaxEncodedValue()-1);
    REQUIRE(v[0] == 6);
    REQUIRE(v[0x4c] == 0);
}

TEST_CASE("Test font with fixed widths")
{
    using sable::Font;
    auto node = getSampleNode()["opening"];
    Font f(node, "opening");
    std::vector<int> v;
    v.reserve(f.getMaxEncodedValue());
    f.getFontWidths(std::back_inserter(v));
    REQUIRE(v.size() == f.getMaxEncodedValue()-1);
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
