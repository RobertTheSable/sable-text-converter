#ifndef SABLE_MAPPING
#define SABLE_MAPPING

namespace Mappers {
    enum Mapper {
        INVALID,
        LOROM,
        EXLOROM
    };
}

static constexpr const int HEADER_LOCATION = 0x80FFC0;
static constexpr const int NORMAL_ROM_MAX_SIZE = 0x400000;

inline int PCToLoROM(int addr, bool header = false, bool high = true)
{
    if (header) {
        addr-=512;
    }
    if (addr<0 || addr>=0x400000) {
        return -1;
    }
    addr=((addr<<1)&0x7F0000)|(addr&0x7FFF)|0x8000;
    if (high) {
        addr|=0x800000;
    }
    return addr;
}
inline int LoROMToPC(int addr, bool header = false)
{
    if (addr<0 || addr>0xFFFFFF ||//not 24bit
                (addr&0xFE0000)==0x7E0000 ||//wram
                (addr&0x408000)==0x000000)//hardware regs
            return -1;
    addr=((addr&0x7F0000)>>1|(addr&0x7FFF));
    if (header) addr+=512;
    return addr;
}

inline int EXLoROMToPC(int addr, bool header = false)
{
    if (addr<0 || addr>0xFFFFFF ||//not 24bit
                (addr&0xFE0000)==0x7E0000 ||//wram
                (addr&0x408000)==0x000000)//hardware regs
            return -1;
    if (addr&0x800000) {
        addr=((addr&0x7F0000)>>1|(addr&0x7FFF));
    } else {
        addr=((addr&0x7F0000)>>1|(addr&0x7FFF))+0x400000;
    }
    return addr;
}
#endif
