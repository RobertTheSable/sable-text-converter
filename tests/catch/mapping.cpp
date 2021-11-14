#include <catch2/catch.hpp>
#include "util.h"

TEST_CASE("Rom mapper class - ToRom")
{
    using sable::util::Mapper;
    Mapper m(sable::util::MapperType::LOROM, false, true, 0x400000);
    SECTION("Lorom - high mode - no header")
    {
        m = Mapper(sable::util::MapperType::LOROM, false, true, 0x400000);
        REQUIRE(m);
        REQUIRE(m.getType() == sable::util::MapperType::LOROM);
        REQUIRE(m.getSize() == 0x400000);
        REQUIRE(m.ToRom(-1) == -1);
        REQUIRE(m.ToRom(0xffffff) == -1);
        REQUIRE(m.ToRom(0x400000) == -1);
        REQUIRE(m.ToRom(0) == 0x808000);
        REQUIRE(m.ToRom(0x8000) == 0x818000);
        REQUIRE(m.ToRom(0x37FFFF) == 0xEFFFFF);
        REQUIRE(m.ToRom(0x3FFFFF) == 0xFFFFFF);
    }
    SECTION("Lorom - high mode - header")
    {
        m = Mapper(sable::util::MapperType::LOROM, true, true, 0x400000);
        REQUIRE(m);
        REQUIRE(m.getSize() == 0x400200);
        REQUIRE(m.ToRom(-1) == -1);
        REQUIRE(m.ToRom(0xffffff) == -1);
        REQUIRE(m.ToRom(0x400200) == -1);
        REQUIRE(m.ToRom(0x400000) == 0xFFFE00);
        REQUIRE(m.ToRom(0x200) == 0x808000);
        REQUIRE(m.ToRom(0x8200) == 0x818000);
        REQUIRE(m.ToRom(0x3801FF) == 0xEFFFFF);
        REQUIRE(m.ToRom(0x4001FF) == 0xFFFFFF);
    }
    SECTION("Lorom - low mode")
    {
        m = Mapper(sable::util::MapperType::LOROM, false, false, 0x400000);
        REQUIRE(m);
        REQUIRE(m.getSize() == 0x400000);
        REQUIRE(m.ToRom(-1) == -1);
        REQUIRE(m.ToRom(0xffffff) == -1);
        REQUIRE(m.ToRom(0) == 0x008000);
        REQUIRE(m.ToRom(0x8000) == 0x018000);
        REQUIRE(m.ToRom(0x37FFFF) == 0x6FFFFF);
        REQUIRE(m.ToRom(0x3FFFFF) == 0xFFFFFF);
    }
    SECTION("Hirom - high mode")
    {
        m = Mapper(sable::util::MapperType::HIROM, false, true, 0x400000);
        REQUIRE(m);
        REQUIRE(m.getSize() == 0x400000);
        REQUIRE(m.ToRom(-1) == -1);
        REQUIRE(m.ToRom(0xffffff) == -1);
        REQUIRE(m.ToRom(0) == 0xC00000);
        REQUIRE(m.ToRom(0x8000) == 0xC08000);
        REQUIRE(m.ToRom(0x37FFFF) == 0xF7FFFF);
        REQUIRE(m.ToRom(0x3FFFFF) == 0xFFFFFF);
    }
    SECTION("Hirom - high mode - header")
    {
        m = Mapper(sable::util::MapperType::HIROM, true, true, 0x400000);
        REQUIRE(m);
        REQUIRE(m.getSize() == 0x400200);
        REQUIRE(m.ToRom(-1) == -1);
        REQUIRE(m.ToRom(0xffffff) == -1);
        REQUIRE(m.ToRom(0x200) == 0xC00000);
        REQUIRE(m.ToRom(0x8200) == 0xC08000);
        REQUIRE(m.ToRom(0x3801FF) == 0xF7FFFF);
        REQUIRE(m.ToRom(0x4001FF) == 0xFFFFFF);
    }
    SECTION("ExLorom")
    {
        SECTION("6m size")
        {
            m = Mapper(sable::util::MapperType::EXLOROM, false, true, 0x600000);

            REQUIRE(m.getSize() == 0x600000);
            REQUIRE(m.ToRom(0x600000) == -1);
        }
        SECTION("6m size - corrected from LoROM")
        {
            m = Mapper(sable::util::MapperType::LOROM, false, true, 0x600000);
            REQUIRE(m.getSize() == 0x600000);
            REQUIRE(m.ToRom(0x600000) == -1);
        }
        SECTION("8m size")
        {
            m = Mapper(sable::util::MapperType::EXLOROM, false, true, 0x800000);
            REQUIRE(m.ToRom(0x600000) == 0x408000);
            REQUIRE(m.ToRom(0x7E0000) == 0x7C8000);
            REQUIRE(m.ToRom(0x7E8000) == 0x7D8000);
            REQUIRE(m.ToRom(0x7F0000) == -2);
            REQUIRE(m.ToRom(0x7F8000) == -2);
        }
        REQUIRE(m);
        REQUIRE(m.getType() == sable::util::MapperType::EXLOROM);
        REQUIRE(m.ToRom(-1) == -1);
        REQUIRE(m.ToRom(0) == 0x808000);
        REQUIRE(m.ToRom(0x400000) == 0x008000);
        REQUIRE(m.ToRom(0x8000) == 0x818000);
        REQUIRE(m.ToRom(0x37FFFF) == 0xEFFFFF);
        REQUIRE(m.ToRom(0x3E0000) == 0xFC8000);
        REQUIRE(m.ToRom(0x3F0000) == 0xFE8000);
        REQUIRE(m.ToRom(0x3FFFFF) == 0xFFFFFF);
        REQUIRE(m.ToRom(0x5fffff) == 0x3FFFFF);
    }
    SECTION("ExHirom")
    {
        SECTION("6m size")
        {
            m = Mapper(sable::util::MapperType::EXHIROM, false, true, 0x600000);
            REQUIRE(m.ToRom(0x600000) == -1);
        }
        SECTION("6m size - corrected from HiROM")
        {
            m = Mapper(sable::util::MapperType::HIROM, false, true, 0x600000);
            REQUIRE(m.getSize() == 0x600000);
            REQUIRE(m.ToRom(0x600000) == -1);
        }
        SECTION("8m size")
        {
            m = Mapper(sable::util::MapperType::EXHIROM, false, true, 0x800000);
            REQUIRE(m.ToRom(0x600000) == 0x600000);
            REQUIRE(m.ToRom(0x7DFFFF) == 0x7DFFFF);
            REQUIRE(m.ToRom(0x7E0000) == -2);
            REQUIRE(m.ToRom(0x7E8000) == 0x3E8000);
            REQUIRE(m.ToRom(0x7F0000) == -2);
            REQUIRE(m.ToRom(0x7F8000) == 0x3F8000);
        }
        REQUIRE(m.getType() == sable::util::MapperType::EXHIROM);
        REQUIRE(m.ToRom(-1) == -1);
        REQUIRE(m.ToRom(0) == 0xC00000);
        REQUIRE(m.ToRom(0x8000) == 0xC08000);
        REQUIRE(m.ToRom(0x37FFFF) == 0xF7FFFF);
        REQUIRE(m.ToRom(0x3E0000) == 0xFE0000);
        REQUIRE(m.ToRom(0x3F0000) == 0xFF0000);
        REQUIRE(m.ToRom(0x3FFFFF) == 0xFFFFFF);
        REQUIRE(m.ToRom(0x400000) == 0x400000);
        REQUIRE(m.ToRom(0x5fffff) == 0x5FFFFF);
    }
    SECTION("Invalid options")
    {
        m = Mapper(sable::util::MapperType::EXLOROM, false, true, 0x800001);
        REQUIRE(!m);
    }
}

TEST_CASE("Rom mapper class - ToPC")
{
    using sable::util::Mapper;
    Mapper m(sable::util::MapperType::LOROM, false, true, 0x400000);
    SECTION("Lorom")
    {
        SECTION("High mode")
        {
            m = Mapper(sable::util::MapperType::LOROM, false, true, 0x400000);
        }
        SECTION("Low mode")
        {
            m = Mapper(sable::util::MapperType::LOROM, false, false, 0x400000);
        }
        REQUIRE(m);
        REQUIRE(m.ToPC(0x808000) == 0);
        REQUIRE(m.ToPC(0x008000) == 0);
        REQUIRE(m.ToPC(0x818000) == 0x8000);
        REQUIRE(m.ToPC(0xEFFFFF) == 0x37FFFF);
        REQUIRE(m.ToPC(0xFFFFFF) == 0x3FFFFF);
        REQUIRE(m.ToPC(0x700000) == -1);
        REQUIRE(m.ToPC(0x708000) == 0x380000);
        REQUIRE(m.ToPC(0xF08000) == 0x380000);
    }
    SECTION("ExLorom")
    {
        SECTION("6m size")
        {
            m = Mapper(sable::util::MapperType::EXLOROM, false, true, 0x600000);
            REQUIRE(m.ToPC(0x408000) == -2);
        }
        SECTION("6m size - corrected from LoROM")
        {
            m = Mapper(sable::util::MapperType::LOROM, false, true, 0x600000);
        }
        SECTION("8m size")
        {
            m = Mapper(sable::util::MapperType::EXLOROM, false, true, sable::util::ROM_MAX_SIZE);
            REQUIRE(m.ToPC(0x408000) == 0x600000);
            REQUIRE(m.ToPC(0x3E8000) == 0x5F0000);
            REQUIRE(m.ToPC(0x7D8000) == 0x7E8000);
            REQUIRE(m.ToPC(0x708000) == 0x780000);
        }
        REQUIRE(m);
        REQUIRE(m.ToPC(0x808000) == 0);
        REQUIRE(m.ToPC(0x008000) == 0x400000);
        REQUIRE(m.ToPC(0x700000) == -1);
        REQUIRE(m.ToPC(0xF08000) == 0x380000);
    }
    SECTION("Hirom")
    {
        m = Mapper(sable::util::MapperType::HIROM, false, true, 0x400000);
        REQUIRE(m);
        REQUIRE(m.ToPC(0xC00000) == 0);
        REQUIRE(m.ToPC(0x400000) == 0);
        REQUIRE(m.ToPC(0x808000) == 0x8000);
        REQUIRE(m.ToPC(0x818000) == 0x18000);
        REQUIRE(m.ToPC(0xEFFFFF) == 0x2FFFFF);
        REQUIRE(m.ToPC(0xFFFFFF) == 0x3FFFFF);
        REQUIRE(m.ToPC(0x700000) == 0x300000);
        REQUIRE(m.ToPC(0x708000) == 0x308000);
        REQUIRE(m.ToPC(0xF08000) == 0x308000);
    }
    SECTION("ExHirom")
    {
        SECTION("6m size")
        {
            m = Mapper(sable::util::MapperType::EXHIROM, false, true, 0x600000);
            REQUIRE(m.ToPC(0x600000) == -2);
        }
        SECTION("8m size")
        {
            m = Mapper(sable::util::MapperType::EXHIROM, false, true, sable::util::ROM_MAX_SIZE);
            REQUIRE(m.ToPC(0x3E8000) == 0x7E8000);
        }
        REQUIRE(m.ToPC(0xC00000) == 0);
        REQUIRE(m.ToPC(0xC08000) == 0x8000);
        REQUIRE(m.ToPC(0xF7FFFF) == 0x37FFFF);
        REQUIRE(m.ToPC(0xFF0000) == 0x3F0000);
        REQUIRE(m.ToPC(0xFFFFFF) == 0x3FFFFF);
        REQUIRE(m.ToPC(0x400000) == 0x400000);
        REQUIRE(m.ToPC(0x5fffff) == 0x5FFFFF);
        REQUIRE(m.ToPC(0x008000) == 0x408000);
    }
    REQUIRE(m.ToPC(-1) == -1);
    REQUIRE(m.ToPC(0x1ffffff) == -1);
    REQUIRE(m.ToPC(0x7fffff) == -1);
    REQUIRE(m.ToPC(0x800000) == -1);
    REQUIRE(m.ToPC(0) == -1);
    REQUIRE(m.ToPC(0x7e0000) == -1);
    REQUIRE(m.ToPC(0x800000) == -1);
}

TEST_CASE("Expanded type tests")
{
    using sable::util::MapperType;
    REQUIRE(sable::util::getExpandedType(MapperType::LOROM) == MapperType::EXLOROM);
    REQUIRE(sable::util::getExpandedType(MapperType::HIROM) == MapperType::EXHIROM);
    REQUIRE(sable::util::getExpandedType(MapperType::EXLOROM) == MapperType::EXLOROM);
    REQUIRE(sable::util::getExpandedType(MapperType::HIROM) == MapperType::EXHIROM);
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
    SECTION("Calculated from max address - LoROM")
    {
        sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
        REQUIRE(m.calculateFileSize(m.ToRom(1)) == 131072);
        REQUIRE(m.calculateFileSize(m.ToRom(262143)) == 262144);
        REQUIRE(m.calculateFileSize(m.ToRom(262144)) == 524288);
        REQUIRE(m.calculateFileSize(m.ToRom(0x300000)) == 0x380000);
        sable::util::Mapper m2(sable::util::MapperType::EXLOROM, false, true, sable::util::MAX_ALLOWED_FILESIZE_SHORTCUT);
        REQUIRE(m.calculateFileSize(m2.ToRom(0x400000)) == 131072);
        REQUIRE_THROWS(m.calculateFileSize(0x7e0000));
    }
    SECTION("Calculated from max address - ExLoROM")
    {
        sable::util::Mapper m(sable::util::MapperType::EXLOROM, false, true, sable::util::MAX_ALLOWED_FILESIZE_SHORTCUT);
        REQUIRE(m.calculateFileSize(m.ToRom(0x400000)) == 0x600000);
        REQUIRE(m.calculateFileSize(m.ToRom(0x600000)) == 0x7F0000);
    }

    SECTION("File size string generation")
    {
        REQUIRE(getFileSizeString(128 * 1024) == "128kb");
        REQUIRE(getFileSizeString(2 * 1024 * 1024) == "2mb");
        REQUIRE(getFileSizeString(((2*1024) + 512) * 1024) == "2.5mb");
        REQUIRE(getFileSizeString(sable::util::ROM_MAX_SIZE) == "8mb");
    }
}
