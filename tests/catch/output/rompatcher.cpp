#include <catch2/catch.hpp>
#include "output/rompatcher.h"
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
    SECTION("LoROM")
    {
        RomPatcher r(sable::util::MapperType::LOROM);
        sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
        SECTION("Resizing to normal ROM size.")
        {
            r.loadRom("sample.sfc", "patch test", -1);
            REQUIRE(r.getRealSize() == 131072);

            REQUIRE_NOTHROW(r.expand(m.calculateFileSize(0xE08001), m));
            REQUIRE(r.getRealSize() == 0x380000);
        }
        SECTION("Resizing to EXLOROM ROM size.")
        {
            SECTION("Base LOROM mapper.")
            {
                r = RomPatcher(sable::util::MapperType::LOROM);
            }
            SECTION("Corrected base size from EXLOROM > LOROM.")
            {
                r = RomPatcher(sable::util::MapperType::EXLOROM);
            }
            r.loadRom("sample.sfc", "patch test", -1);
            sable::util::Mapper m2(sable::util::MapperType::EXLOROM, false, true, 0x600000);
            REQUIRE(m.ToPC(sable::util::HEADER_LOCATION) == 0x007FC0);
            unsigned char test = r.at(m.ToPC(sable::util::HEADER_LOCATION));
            unsigned char mode = r.at(m.ToPC(sable::util::HEADER_LOCATION) + 0x15);
            REQUIRE_NOTHROW(r.expand(m2.calculateFileSize(0x208000), m2));
            REQUIRE(r.getRealSize() == 0x600000);
            REQUIRE(m2.ToPC(sable::util::HEADER_LOCATION) == 0x400000+m.ToPC(sable::util::HEADER_LOCATION));
            REQUIRE(r.at(m2.ToPC(sable::util::HEADER_LOCATION)) == test);
            REQUIRE(r.at(m2.ToPC(sable::util::HEADER_LOCATION) + 0x15) == mode);
        }
    }
    SECTION("Resizing to EXHIROM ROM size.")
    {
        RomPatcher r(sable::util::MapperType::HIROM);
        SECTION("Base HIROM mapper.")
        {
            r = RomPatcher(sable::util::MapperType::HIROM);
        }
        SECTION("Corrected base size from EXHIROM > HIROM.")
        {
            r = RomPatcher(sable::util::MapperType::EXHIROM);
        }

        sable::util::Mapper m(sable::util::MapperType::HIROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
        sable::util::Mapper m2(sable::util::MapperType::EXHIROM, false, true, 0x600000);
        r.loadRom("sample.sfc", "patch test", -1);

        REQUIRE(m.ToPC(sable::util::HEADER_LOCATION) == 0x00FFC0);
        REQUIRE(m2.ToPC(sable::util::HEADER_LOCATION) == 0x400000+m.ToPC(sable::util::HEADER_LOCATION));

        unsigned char test = r.at(m.ToPC(sable::util::HEADER_LOCATION));
        unsigned char mode = r.at(m.ToPC(sable::util::HEADER_LOCATION) + 0x15);
        r.expand(m2.calculateFileSize(0x408000), m2);
        REQUIRE(r.getRealSize() == 0x600000);
        REQUIRE(r.at(m2.ToPC(sable::util::HEADER_LOCATION)) == test);
        REQUIRE(r.at(m2.ToPC(sable::util::HEADER_LOCATION) + 0x15) == (mode | 4) );
    }
}

TEST_CASE("Expansion error checking.", "[rompatcher]")
{
    SECTION("HIROM > EXLOROM throws logic error")
    {
        RomPatcher r(sable::util::MapperType::HIROM);
        sable::util::Mapper m(sable::util::MapperType::EXLOROM, false, true, 0x600000);
        REQUIRE_THROWS_AS(r.expand(0x60000, m), std::logic_error);
    }
    SECTION("LoROM > EXHiROM throws logic error")
    {
        RomPatcher r(sable::util::MapperType::LOROM);
        sable::util::Mapper m(sable::util::MapperType::EXHIROM, false, true, 0x600000);
        REQUIRE_THROWS_AS(r.expand(0x60000, m), std::logic_error);
    }
}


TEST_CASE("Asar patch testing", "[rompatcher]")
{
    using sable::RomPatcher;
    RomPatcher r(sable::util::MapperType::LOROM);
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    r.loadRom("sample.sfc", "patch test", -1);
    r.expand(m.calculateFileSize(0xE08000), m);
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
