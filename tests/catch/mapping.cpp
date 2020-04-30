#include <catch2/catch.hpp>
#include "util.h"

TEST_CASE("Test LoROM to PC Results")
{
    using sable::util::ROMToPC, sable::util::Mapper;
    REQUIRE(ROMToPC(Mapper::LOROM, 0x80FFC0) == 0x007FC0);
    REQUIRE(ROMToPC(Mapper::LOROM, sable::util::HEADER_LOCATION) == 0x007FC0);
    REQUIRE(ROMToPC(Mapper::LOROM, 0xee8000) == 0x370000);
    REQUIRE(ROMToPC(Mapper::LOROM, 0x6e8000) == 0x370000);
    REQUIRE(ROMToPC(Mapper::LOROM, -1) == -1);
    REQUIRE(ROMToPC(Mapper::LOROM, 0) == -1);
    REQUIRE(ROMToPC(Mapper::LOROM, 0x7E0000) == -1);
    REQUIRE(ROMToPC(Mapper::LOROM, 0x800000) == -1);
}

TEST_CASE("Test PC to LoROM Results")
{
    using sable::util::PCtoRom, sable::util::Mapper;
    REQUIRE(PCtoRom(Mapper::LOROM, 0x0) == 0x808000);
    REQUIRE(PCtoRom(Mapper::LOROM, 0x370000) == 0xee8000);
    REQUIRE(PCtoRom(Mapper::LOROM, sable::util::NORMAL_ROM_MAX_SIZE) == -1);
}

TEST_CASE("Test ExLoROM to PC Results")
{
    using sable::util::PCtoRom, sable::util::Mapper;
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0x80FFC0) == 0x007FC0);
    REQUIRE(ROMToPC(Mapper::EXLOROM, sable::util::HEADER_LOCATION) == 0x407FC0);
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0xee8000) == 0x370000);
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0x6e8000) == 0x770000);
    REQUIRE(ROMToPC(Mapper::EXLOROM, -1) == -1);
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0x7E0000) == -1);
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0x800000) == -1);
}

TEST_CASE("Test PC to ExLoROM Results")
{
    using sable::util::PCtoRom, sable::util::Mapper;
    REQUIRE(PCtoRom(Mapper::EXLOROM, 0x0) == 0x808000);
    REQUIRE(PCtoRom(Mapper::EXLOROM, 0x370000) == 0xee8000);
    REQUIRE(PCtoRom(Mapper::EXLOROM, sable::util::NORMAL_ROM_MAX_SIZE) == 0x008000);
    REQUIRE(PCtoRom(Mapper::EXLOROM, sable::util::ROM_MAX_SIZE) == -1);
    REQUIRE(PCtoRom(Mapper::EXLOROM, sable::util::ROM_MAX_SIZE-1) == 0x7DFFFF);
}


TEST_CASE("String to hex conversion")
{
    using sable::util::strToHex;
    REQUIRE(strToHex("0") == std::make_pair<unsigned int, int>(0, 1));
    REQUIRE(strToHex("0000") == std::make_pair<unsigned int, int>(0, 2));
    REQUIRE(strToHex("000000") == std::make_pair<unsigned int, int>(0, 3));
    REQUIRE(strToHex("$000000") == std::make_pair<unsigned int, int>(0, 3));
    REQUIRE(strToHex("$FF") == std::make_pair<unsigned int, int>(255, 1));
    REQUIRE(strToHex("XYV") == std::make_pair<unsigned int, int>(0, -1));
    REQUIRE_THROWS(strToHex("$1000000"));
}

TEST_CASE("File size calculation.")
{
    using sable::util::calculateFileSize, sable::util::getFileSizeString;
    SECTION("Calculated from config.")
    {
        REQUIRE(calculateFileSize("2048") == 2048);
        REQUIRE(calculateFileSize("2KB") == 2048);
        REQUIRE(calculateFileSize("2 kb ") == 2048);
        REQUIRE(calculateFileSize("2M") == 2097152);
        REQUIRE(calculateFileSize("2MB") == calculateFileSize("2mb"));
        REQUIRE(calculateFileSize("2MB") == calculateFileSize("2M"));
        REQUIRE(calculateFileSize("2KB") == calculateFileSize("2K"));
        REQUIRE(calculateFileSize("3.5MB") == 3670016);
        REQUIRE(calculateFileSize("2GB") == 0);
        REQUIRE(calculateFileSize("10mb") == 0);
        REQUIRE(calculateFileSize("2.5") == 0);
        REQUIRE(calculateFileSize("") == 0);
        REQUIRE(calculateFileSize("test") == 0);
        REQUIRE(calculateFileSize("2M test") == 0);
    }
    SECTION("Calculated from max address")
    {
        REQUIRE(calculateFileSize(sable::util::PCToLoROM(1)) == 131072);
        REQUIRE(calculateFileSize(sable::util::PCToLoROM(262143)) == 262144);
        REQUIRE(calculateFileSize(sable::util::PCToLoROM(262144)) == 524288);
        REQUIRE(calculateFileSize(sable::util::PCToLoROM(0x300000)) == 0x380000);
        REQUIRE(calculateFileSize(sable::util::PCToEXLoROM(0x400000)) == 131072);
        REQUIRE(calculateFileSize(
                    sable::util::PCToEXLoROM(0x400000),
                    sable::util::Mapper::EXLOROM
                    ) == 0x600000);
        REQUIRE(calculateFileSize(
                    sable::util::PCToEXLoROM(0x600000),
                    sable::util::Mapper::EXLOROM
                    ) == 0x7F0000);
        REQUIRE_THROWS(calculateFileSize(0x7e0000));
    }
    SECTION("File size string generation")
    {
        REQUIRE(getFileSizeString(128 * 1024) == "128kb");
        REQUIRE(getFileSizeString(2 * 1024 * 1024) == "2mb");
        REQUIRE(getFileSizeString(((2*1024) + 512) * 1024) == "2.5mb");
        REQUIRE(getFileSizeString(sable::util::ROM_MAX_SIZE) == "8mb");
    }
}
