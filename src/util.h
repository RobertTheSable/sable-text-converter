#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include "mapping.h"

namespace sable {
namespace util {

static constexpr size_t MAX_ALLOWED_FILESIZE = 8388608;

typedef std::vector<unsigned char> ByteVector;
std::pair<unsigned int, int> strToHex(const std::string& val);
int PCToLoROM(int addr, bool header = false, bool high = true);
int PCToEXLoROM(int addr, bool header = false);
int LoROMToPC(int addr, bool header = false);
int EXLoROMToPC(int addr, bool header = false);
int ROMToPC(Mapper mapType, int addr, bool header = false);
int PCtoRom(Mapper mapType, int addr, bool header = false);
size_t calculateFileSize(const std::string& value);
Mapper getExpandedType(Mapper m);
}

}

#endif // UTIL_H
