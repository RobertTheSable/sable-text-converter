#include <catch2/catch.hpp>
#include "output/formatter.h"

#include "wrapper/filesystem.h"


TEST_CASE("Basic generation functions", "[rompatcher]")
{
    using namespace sable::formatter;
    REQUIRE(generateDefine("test") == "!test");
    REQUIRE((generateInclude(fs::temp_directory_path() / "test" / "test.txt", fs::path(), false)) == "incsrc " + (fs::temp_directory_path() / "test" / "test.txt").generic_string());
    REQUIRE((generateInclude(fs::temp_directory_path() / "test" / "test.txt", fs::temp_directory_path(), false)) == "incsrc test/test.txt");
    REQUIRE(generateAssignment("test", 0x8081, 1) == "!test = $81");
    REQUIRE(generateAssignment("test", 0x8000, 2) == "!test = $8000");
    REQUIRE(generateAssignment("test", 0x8000, 3) == "!test = $008000");
    REQUIRE(generateAssignment("test", 0x10, 1, 10) == "!test = 16");
    REQUIRE(generateAssignment("test", 0x10, 4, 2) == "!test = %00000000000000000000000000010000");
    REQUIRE(generateAssignment("test", 0x10, 3, 2) == "!test = %000000000000000000010000");
    REQUIRE(generateAssignment("test", 0x10, 2, 2) == "!test = %0000000000010000");
    REQUIRE(generateAssignment("test", 0x10, 1, 2) == "!test = %00010000");
    REQUIRE(generateAssignment("test", 0x10, 1, 10, "testBase") == "!test = !testBase+16");
    REQUIRE_THROWS(generateAssignment("test", 0x10, 5));
    REQUIRE_THROWS(generateAssignment("test", 0x10, 4, 9));
    REQUIRE_THROWS(generateAssignment("test", 0x10, 5, 2));
}
