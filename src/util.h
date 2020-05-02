#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

namespace sable {
namespace util {

// TODO: hirom support
enum Mapper {
    INVALID,
    LOROM,
    EXLOROM
};

static constexpr const int HEADER_LOCATION = 0x00FFC0;
static constexpr const int NORMAL_ROM_MAX_SIZE = 0x400000;
static constexpr const int ROM_MAX_SIZE = 0x7F0000;

static constexpr size_t MAX_ALLOWED_FILESIZE = 8388608;

typedef std::vector<unsigned char> ByteVector;
std::pair<unsigned int, int> strToHex(const std::string& val);
int PCToLoROM(int addr, bool header = false, bool high = true);
int PCToEXLoROM(int addr, bool header = false);
int LoROMToPC(int addr, bool header = false);
int EXLoROMToPC(int addr, bool header = false);
int ROMToPC(Mapper mapType, int addr, bool header = false);
int PCToROM(Mapper mapType, int addr, bool header = false, bool high = true);
size_t calculateFileSize(const std::string& value);
size_t calculateFileSize(int maxAddress, Mapper m = Mapper::LOROM);
std::string getFileSizeString(int value);
Mapper getExpandedType(Mapper m);
}

}

#endif // UTIL_H
