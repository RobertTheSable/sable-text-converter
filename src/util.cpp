#include "util.h"
#include <sstream>
#include <exception>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>

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

int sable::util::ROMToPC(sable::util::Mapper mapType, int addr, bool header)
{
    if (mapType == Mapper::LOROM) {
        return LoROMToPC(addr, header);
    } else if (mapType == Mapper::EXLOROM) {
        return EXLoROMToPC(addr, header);
    } else {
        return -1;
    }
}

int sable::util::PCToROM(sable::util::Mapper mapType, int addr, bool header, bool high)
{
    if (mapType == Mapper::LOROM) {
        return PCToLoROM(addr, header, high);
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

sable::util::Mapper sable::util::getExpandedType(sable::util::Mapper m)
{
    if (m == Mapper::LOROM) {
        return Mapper::EXLOROM;
    }
    return m;
}

size_t sable::util::calculateFileSize(const std::string &value)
{
    size_t returnVal = 0;
    if (!value.empty()) {
        std::istringstream stream(value);
        uint64_t calculatedValue = 0;
        if (stream >> calculatedValue) {
            calculatedValue *= 1024;
            if (!stream.eof()) {
                if (stream.peek() == '.') {
                    stream.get();
                    uint64_t tempValue = 0;
                    if (stream >> tempValue) {
                        calculatedValue += (tempValue * 1024) / 10;
                    }
                }
                std::string sizeIndicator = "";
                if (stream >> sizeIndicator) {
                    std::transform(sizeIndicator.begin(), sizeIndicator.end(), sizeIndicator.begin(), ::tolower);
                    if (sizeIndicator == "kb" || sizeIndicator == "k") {
                        calculatedValue = calculatedValue * 1024;
                    } else if ((sizeIndicator == "mb") || (sizeIndicator == "m")) {
                        calculatedValue = calculatedValue * (1024 * 1024);
                    } else {
                        // other sizes not supported.
                        calculatedValue = 0;
                    }
                }
                if (stream >> sizeIndicator) {
                    calculatedValue = 0;
                }
            }
            if (calculatedValue != 0 && (calculatedValue % (1024 * 1024)) == 0) {
                calculatedValue /= 1024;
                if (calculatedValue <= MAX_ALLOWED_FILESIZE) {
                    if (calculatedValue >= ROM_MAX_SIZE){
                        returnVal = ROM_MAX_SIZE;
                    } else {
                        returnVal = calculatedValue;
                    }
                }
            }
        }
    }
    return returnVal;
}

size_t sable::util::calculateFileSize(int maxAddress, Mapper m)
{
    int address = ROMToPC(m, maxAddress);
    if (address < 0) {
        std::ostringstream errorMsg;
        errorMsg << "Error: address " << std::hex << address
                 << " is negative.";
        throw std::logic_error(errorMsg.str());
    } else if (address >= ROM_MAX_SIZE) {
        std::ostringstream errorMsg;
        errorMsg << "Error: address " << std::hex << address
                 << " is too large for an SNES rom.";
        throw std::logic_error(errorMsg.str());
    } else if (address >= NORMAL_ROM_MAX_SIZE) {
        if (address >= 6291456) {
            return ROM_MAX_SIZE;
        } else {
            return 6291456;
        }
    } else if (address < 131072) {
        return 131072;
    } else if (address < 262144) {
        return 262144;
    } else {
        return (1 + (address / 524288)) * 524288;
    }
}

std::string sable::util::getFileSizeString(int value)
{
    if (value > MAX_ALLOWED_FILESIZE|| value <= 0) {
        return "";
    } else if (value >= ROM_MAX_SIZE) {
        value = MAX_ALLOWED_FILESIZE;
    }
    int returnVal = value / 1024;
    if (returnVal < 1024) {
        return std::to_string(returnVal)+ "kb";
    }
    returnVal /= 1024;
    if ((value/1024) % 1024 == 512) {
        return std::to_string(returnVal) + ".5mb";
    } else {
        return std::to_string(returnVal)+ "mb";
    }

}
