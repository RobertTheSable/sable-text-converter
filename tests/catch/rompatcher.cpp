#include <catch2/catch.hpp>
#include "rompatcher.h"
#include <cstring>
#include <fstream>

TEST_CASE("Expansion Test", "[rompatcher]")
{
    using sable::RomPatcher;
    RomPatcher r("sample.sfc", "patch test", "lorom", -1);
    REQUIRE(r.getRomSize() == 131072);
    REQUIRE(r.getRomSize() == r.getRealSize());
    const char* headerName = "Testing ROM          ";
    REQUIRE(memcmp(
                &r.atROMAddr(HEADER_LOCATION),
                headerName,
                strlen(headerName)
                ) == 0
            );
    r.expand(sable::util::LoROMToPC(0xE08001));
    REQUIRE(r.getRomSize() == 0x380000);
    SECTION("Header can still be read.")
    {
        REQUIRE(memcmp(
                    &r.atROMAddr(HEADER_LOCATION|0x800000),
                    headerName,
                    strlen(headerName)
                    ) == 0
                );
        REQUIRE(r.atROMAddr(0xE08000) == 0);
    }
    SECTION("Rom will not be expanded if the max address is lower than the given address.")
    {
        REQUIRE(r.expand(sable::util::LoROMToPC(0x808000)) == false);
    }
    SECTION("Header is at the correct location in ExLOROM.")
    {
        r.expand(sable::util::EXLoROMToPC(0x600000));
        REQUIRE(r.atROMAddr(0x600000) == 0);
        REQUIRE(r.getRomSize() == ROM_MAX_SIZE);
        REQUIRE(memcmp(
                    &r.atROMAddr(HEADER_LOCATION),
                    headerName,
                    strlen(headerName)
                    ) == 0
                );
    }
}

TEST_CASE("Asar patch testing", "[rompatcher]")
{
    using sable::RomPatcher;
    RomPatcher r("sample.sfc", "patch test", "lorom", -1);
    REQUIRE(r.getRomSize() == 131072);
    r.expand(sable::util::LoROMToPC(0xE08000));
    SECTION("Test that Asar patch can be applied.")
    {
        REQUIRE(r.applyPatchFile("sample.asm") == true);
        REQUIRE(r.atROMAddr(0xE08000) == 2);
        std::vector<std::string> msgs;
        REQUIRE(r.getMessages(std::back_inserter(msgs)));
        REQUIRE(msgs.empty());
    }
    SECTION("Test bad Assar patch.")
    {
        REQUIRE(r.applyPatchFile("bad_test.asm") == false);
        REQUIRE(r.atROMAddr(0xE08000) != 2);
        std::vector<std::string> msgs;
        REQUIRE(r.getMessages(std::back_inserter(msgs)));
        REQUIRE(!msgs.empty());
    }
}
