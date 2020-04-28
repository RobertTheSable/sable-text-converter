#include <catch2/catch.hpp>
#include "rompatcher.h"
#include <cstring>
#include <fstream>

using sable::RomPatcher;

TEST_CASE("Include Generation")
{
    RomPatcher r;
    REQUIRE((r.generateInclude(fs::temp_directory_path() / "test" / "test.txt", fs::path(), false)) == "incsrc " + (fs::temp_directory_path() / "test" / "test.txt").generic_string());
    REQUIRE((r.generateInclude(fs::temp_directory_path() / "test" / "test.txt", fs::temp_directory_path(), false)) == "incsrc test/test.txt");
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
