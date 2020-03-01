#include "util.h"
#include <sstream>
#include <exception>

std::pair<unsigned int, int> sable::util::strToHex(const std::string &val)
{
    int bytes;
    std::stringstream t;
    if (val.front() == '$') {
        bytes = val.length() / 2;
        t = std::stringstream(val.substr(1, val.length()-1));
    } else {
        bytes = (val.length() + 1) / 2;
        t = std::stringstream(val);
    }
    int tmp;
    t >> std::hex >> tmp;

    if (tmp > 0xFFFFFF) {
        throw std::runtime_error(std::string("Hex value \"") + val +"\" too large.");
    }
    if (!t.eof()){
        return std::make_pair(0, -1);
    }
    return std::make_pair(tmp, bytes);
}
int sable::util::PCToLoROM(int addr, bool header, bool high)
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
int sable::util::LoROMToPC(int addr, bool header)
{
    if (addr<0 || addr>0xFFFFFF ||//not 24bit
                (addr&0xFE0000)==0x7E0000 ||//wram
                (addr&0x408000)==0x000000)//hardware regs
            return -1;
    addr=((addr&0x7F0000)>>1|(addr&0x7FFF));
    if (header) addr+=512;
    return addr;
}

int sable::util::EXLoROMToPC(int addr, bool header)
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
    if (header) addr+=512;
    return addr;
}

int sable::util::ROMToPC(Mapper mapType, int addr, bool header)
{
    if (mapType == Mapper::LOROM) {
        return LoROMToPC(addr, header);
    } else if (mapType == Mapper::EXLOROM) {
        return EXLoROMToPC(addr, header);
    } else {
        return -1;
    }
}

int sable::util::PCtoRom(Mapper mapType, int addr, bool header)
{
    if (mapType == Mapper::LOROM) {
        return PCToLoROM(addr, header);
    } else if(mapType == Mapper::EXLOROM){
        return PCToEXLoROM(addr, header);
    } else {
        return -1;
    }
}

int sable::util::PCToEXLoROM(int addr, bool header)
{
    if (addr >= ROM_MAX_SIZE) {
        return -1;
    } else {
        if (addr >= NORMAL_ROM_MAX_SIZE) {
            return PCToLoROM(addr - 0x400000, header, false);
        } else {
            return PCToLoROM(addr, header, true);
        }
    }
}

Mapper sable::util::getExpandedType(Mapper m)
{
    if (m == Mapper::LOROM) {
        return Mapper::EXLOROM;
    }
    return m;
}
