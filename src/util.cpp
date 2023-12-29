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

sable::util::MapperType sable::util::getExpandedType(sable::util::MapperType m)
{
    switch (m) {
        case MapperType::LOROM:
            return MapperType::EXLOROM;
        case MapperType::HIROM:
            return MapperType::EXHIROM;
        default:
            return m;
    }
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
                if (calculatedValue <= MAX_ALLOWED_FILESIZE_SHORTCUT) {
                    returnVal = calculatedValue;
                }
            }
        }
    }
    return returnVal;
}

std::string sable::util::getFileSizeString(int value)
{
    if (value > MAX_ALLOWED_FILESIZE_SHORTCUT || value <= 0) {
        return ""; // not covered
    } else if (value >= ROM_MAX_SIZE) {
        value = MAX_ALLOWED_FILESIZE_SHORTCUT;
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

sable::util::Mapper::Mapper(const sable::util::MapperType &m, bool header, bool highType, int size): shift(0), offset(0), max_size(size), m_type(m)
{
    using sable::util::MapperType;
    if (header) {
        offset = 0x200;
        max_size += 0x200;
    }
    if ((max_size - offset) > 0x400000) {
        m_type = getExpandedType(m_type);
    }

    if (max_size > 0x800000) {
        m_type = MapperType::INVALID;
    } else {
        mask = 0;
        switch (m_type) {
        case MapperType::LOROM:
            if (highType) {
                mask = 0x800000;
            }
            [[fallthrough]];
        case MapperType::EXLOROM:
            shift = 1;
            if (size <= 0x400000) {
                m_type = MapperType::LOROM;
            } else {
                mask = 0x800000;
            }
            mask = mask | 0x8000;
            // LoROM SRAM overlaps with ROMSEL region
            sram_mask = 0x708000;
            break;
        case MapperType::HIROM:
            // lunar address doesn't have a separate "high" hirom type but it seems like it's theoretically possible
            if (highType) {
                mask = 0x800000;
            }
            [[fallthrough]];
        case MapperType::EXHIROM:
            shift = 0;
            if (size <= 0x400000) {
                m_type = MapperType::HIROM;
            } else {
                mask = 0x800000;
            }
            mask = mask | 0x400000;
            // HiROM SRAM doesn't overlap with ROMSEL region
            sram_mask = 0;
            break;
        default:
            break;
        }
    }
}

int sable::util::Mapper::ToRom(int address) const
{
    if (address < 0 || address >= max_size) {
        // cannot exist in the ROM
        return -1;
    }
    int adjustedAddress = (address - offset);
    int sizeMask =  ((adjustedAddress << 1) ^ (mask)) & 0x800000;
    int bank = ((adjustedAddress << shift) | mask) & 0x7F0000;
    int shortAddress = (adjustedAddress | mask) & 0xFFFF;
    int finalAddress = shortAddress | bank | sizeMask;

    // Prevent returning an address that points to WRAM.
    if ((finalAddress & 0xFE0000) == 0x7E0000) {
        // ExLoROM can use 0x7E00-0x7EFF
        // ExHiROM can use 0x7E00-0x7E7F and 0x7F00-0x7F7F
        if (((~mask & 0x800000) | (finalAddress & ((~mask & 0x8000) << shift))) == 0) {
            // area can exist in the ROM but is not mapped
            return -2;
        }
        finalAddress ^= 0x400000 << shift;
    }
    return finalAddress;
}

int sable::util::Mapper::ToPC(int address) const
{
    if (address<0 || address>0xFFFFFF) //not 24bit
        return -1;
    if ((address&0xFE0000)==0x7E0000) //wram
        return -1;
    if ((address&0x408000)==0x000000)//hardware regs
        return -1;
    if ((address&sram_mask)==0x700000) //sram (low parts of banks 70-7D)
        return -1;
    int reverseAddressMask = (~mask) & 0xFFFF;
    int reverseBankMask = (~mask) & 0x7F0000;
    int reverseSizeMask = ((max_size - 1) & (0x800000 & ~address) >> 1) << shift;
    int bank = ((address & reverseBankMask) | reverseSizeMask) >> shift;
    int calculatedAddress = ((address & reverseAddressMask) | bank) + offset;
    if (calculatedAddress >= max_size) {
        return -2;
    }
    return calculatedAddress;
}

sable::util::MapperType sable::util::Mapper::getType() const
{
    return m_type;
}


sable::util::Mapper::operator bool() const
{
    return m_type != MapperType::INVALID;
}


size_t sable::util::Mapper::calculateFileSize(int maxAddress) const
{
    int address = ToPC(maxAddress);
    if (address < 0) {
        std::ostringstream errorMsg;
        errorMsg << "Error: address " << std::hex << address
                 << " is negative.";
        throw std::logic_error(errorMsg.str());
    } else if (address >= ROM_MAX_SIZE) { // not covered by tests, should be unreachable?
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

void sable::util::Mapper::setIsHeadered(bool isHeadered) // not covered
{
    offset = isHeadered ? 0x200 : 0;
}

int sable::util::Mapper::getSize() const
{
    return max_size;
}

int sable::util::Mapper::skipToNextBank(int address) const
{
    int mirrorMask = 0;

    if (getSize() <= util::NORMAL_ROM_MAX_SIZE) {
        mirrorMask = (~address & 0x800000);
    }
    return ToRom(ToPC(address| 0xFFFF) +1) ^ mirrorMask;
}
