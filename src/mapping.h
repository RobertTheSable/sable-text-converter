#ifndef SABLE_MAPPING
#define SABLE_MAPPING

enum Mapper {
    INVALID,
    LOROM,
    EXLOROM
};

static constexpr const int HEADER_LOCATION = 0x00FFC0;
static constexpr const int NORMAL_ROM_MAX_SIZE = 0x400000;
static constexpr const int ROM_MAX_SIZE = 0x7F0000;
#endif
