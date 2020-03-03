#ifndef CACHE_H
#define CACHE_H

#include "wrapper/filesystem.h"

namespace sable {
class Cache
{
static constexpr const int MAX_ADDR = 0;
public:
    Cache(const std::string& path = "");
    int getMaxAddress() const;
    void setMaxAddress(int value);

    bool isReadable() const;
    bool write() const;

private:
    int m_MaxAddress;
    bool m_IsReadable;
    fs::path m_CacheFile;
};
}


#endif // CACHE_H
