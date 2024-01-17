#ifndef SABLE_UTIL_MAPPER_H
#define SABLE_UTIL_MAPPER_H

#include <cstdint>
#include <optional>
#include <string>

namespace sable {
namespace util {

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
static constexpr std::size_t MAX_ALLOWED_FILESIZE_SHORTCUT = 8388608;

struct ParsedHex {
    unsigned int value;
    int length;
};

std::optional<ParsedHex> strToHex(const std::string& val);

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

} // namespace util
} // namespace sable

#endif // SABLE_UTIL_MAPPER_H
