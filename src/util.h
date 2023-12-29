#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

namespace sable {
namespace util {

// TODO: hirom support
enum MapperType {
    INVALID,
    LOROM,
    EXLOROM,
    HIROM,
    EXHIROM
};

static constexpr const int HEADER_LOCATION = 0x00FFC0;
static constexpr const int NORMAL_ROM_MAX_SIZE = 0x400000;
static constexpr const int ROM_MAX_SIZE = 0x007F0000;

static constexpr size_t MAX_ALLOWED_FILESIZE_SHORTCUT = 8388608;

typedef std::vector<unsigned char> ByteVector;
std::pair<unsigned int, int> strToHex(const std::string& val);
size_t calculateFileSize(const std::string& value);
std::string getFileSizeString(int value);
MapperType getExpandedType(MapperType m);
class Mapper {
    int shift;
    int offset;
    int max_size;
    unsigned int mask;
    unsigned int sram_mask;
    MapperType m_type;
public:
    Mapper(const MapperType& m, bool header, bool highType, int size = 0);
    int ToRom(int address) const;
    int ToPC(int address) const;
    operator bool() const;
    MapperType getType() const;
    size_t calculateFileSize(int maxAddress) const;
    void setIsHeadered(bool isHeadered);
    int getSize() const;
    int skipToNextBank(int address) const;
};
}

}
#endif // UTIL_H
