#include <catch2/catch.hpp>
#include "util.h"

TEST_CASE("Test LoROM to PC Results")
{
    using sable::util::ROMToPC;
    REQUIRE(ROMToPC(Mapper::LOROM, 0x80FFC0) == 0x007FC0);
    REQUIRE(ROMToPC(Mapper::LOROM, HEADER_LOCATION) == 0x007FC0);
    REQUIRE(ROMToPC(Mapper::LOROM, 0xee8000) == 0x370000);
    REQUIRE(ROMToPC(Mapper::LOROM, 0x6e8000) == 0x370000);
    REQUIRE(ROMToPC(Mapper::LOROM, -1) == -1);
    REQUIRE(ROMToPC(Mapper::LOROM, 0) == -1);
    REQUIRE(ROMToPC(Mapper::LOROM, 0x7E0000) == -1);
    REQUIRE(ROMToPC(Mapper::LOROM, 0x800000) == -1);
}

TEST_CASE("Test PC to LoROM Results")
{
    using sable::util::PCtoRom;
    REQUIRE(PCtoRom(Mapper::LOROM, 0x0) == 0x808000);
    REQUIRE(PCtoRom(Mapper::LOROM, 0x370000) == 0xee8000);
    REQUIRE(PCtoRom(Mapper::LOROM, NORMAL_ROM_MAX_SIZE) == -1);
}

TEST_CASE("Test ExLoROM to PC Results")
{
    using sable::util::ROMToPC;
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0x80FFC0) == 0x007FC0);
    REQUIRE(ROMToPC(Mapper::EXLOROM, HEADER_LOCATION) == 0x407FC0);
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0xee8000) == 0x370000);
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0x6e8000) == 0x770000);
    REQUIRE(ROMToPC(Mapper::EXLOROM, -1) == -1);
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0x7E0000) == -1);
    REQUIRE(ROMToPC(Mapper::EXLOROM, 0x800000) == -1);
}

TEST_CASE("Test PC to ExLoROM Results")
{
    using sable::util::PCtoRom;
    REQUIRE(PCtoRom(Mapper::EXLOROM, 0x0) == 0x808000);
    REQUIRE(PCtoRom(Mapper::EXLOROM, 0x370000) == 0xee8000);
    REQUIRE(PCtoRom(Mapper::EXLOROM, NORMAL_ROM_MAX_SIZE) == 0x008000);
    REQUIRE(PCtoRom(Mapper::EXLOROM, ROM_MAX_SIZE) == -1);
    REQUIRE(PCtoRom(Mapper::EXLOROM, ROM_MAX_SIZE-1) == 0x7DFFFF);
}

TEST_CASE("File size calculation.")
{
    using sable::util::calculateFileSize;
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
