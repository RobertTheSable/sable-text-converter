#include <catch2/catch.hpp>
#include "rompatcher.h"
#include <cstring>
#include <fstream>

using sable::RomPatcher;

TEST_CASE("Basic generation functions")
{
    RomPatcher r;
    REQUIRE((r.generateInclude(fs::temp_directory_path() / "test" / "test.txt", fs::path(), false)) == "incsrc " + (fs::temp_directory_path() / "test" / "test.txt").generic_string());
    REQUIRE((r.generateInclude(fs::temp_directory_path() / "test" / "test.txt", fs::temp_directory_path(), false)) == "incsrc test/test.txt");
    REQUIRE(r.generateAssignment("test", 0x8000, 2) == "!test = $8000");
    REQUIRE(r.generateAssignment("test", 0x8000, 3) == "!test = $008000");
    REQUIRE(r.generateAssignment("test", 0x10, 1, 10) == "!test = 16");
    REQUIRE(r.generateAssignment("test", 0x10, 1, 2) == "!test = %00010000");
    REQUIRE(r.generateAssignment("test", 0x10, 1, 10, "testBase") == "!test = !testBase+16");
    REQUIRE_THROWS(r.generateAssignment("test", 0x10, 5));
    REQUIRE_THROWS(r.generateAssignment("test", 0x10, 4, 9));
}

TEST_CASE("Expansion Test", "[rompatcher]")
{
    using sable::RomPatcher;
    RomPatcher r;
    r.loadRom("sample.sfc", "patch test", -1);
    REQUIRE(r.getRealSize() == 131072);

    r.expand(sable::util::LoROMToPC(0xE08001));
    REQUIRE(r.getRealSize() == 0x380000);
}


TEST_CASE("Asar patch testing", "[rompatcher]")
{
    using sable::RomPatcher;
    RomPatcher r;
    r.loadRom("sample.sfc", "patch test", -1);
    r.expand(sable::util::LoROMToPC(0xE08000));
    SECTION("Test that Asar patch can be applied.")
    {
        REQUIRE(r.applyPatchFile("sample.asm") == true);
        std::vector<std::string> msgs;
        REQUIRE(r.getMessages(std::back_inserter(msgs)));
        REQUIRE(msgs.empty());
    }
    SECTION("Test bad Assar patch.")
    {
        REQUIRE(r.applyPatchFile("bad_test.asm") == false);
        std::vector<std::string> msgs;
        REQUIRE(r.getMessages(std::back_inserter(msgs)));
        REQUIRE(!msgs.empty());
    }
}
