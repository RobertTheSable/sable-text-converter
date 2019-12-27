#include "cache.h"
#include <fstream>

SableCache::SableCache() : m_MaxAddress(0), m_IsReadable(false)
{
    m_CacheFile = fs::path("cache") / "cache.bin";
    if (fs::exists(m_CacheFile)) {
        std::ifstream input(m_CacheFile.string(), std::ios::binary);
        input.read(reinterpret_cast<char*>(&m_MaxAddress), sizeof (int));
        input.close();
        m_IsReadable = true;
    }
}

int SableCache::getMaxAddress() const
{
    if (m_IsReadable) {
        return m_MaxAddress;
    } else {
        return 0;
    }
}

void SableCache::setMaxAddress(int value)
{
    m_MaxAddress = value;
    m_IsReadable = true;
}

bool SableCache::isReadable() const
{
    return m_IsReadable;
}

bool SableCache::write() const
{
    if (m_IsReadable) {
        if (!fs::exists(m_CacheFile.parent_path())) {
            fs::create_directory(m_CacheFile.parent_path());
        }
        if (fs::is_directory(m_CacheFile.parent_path())) {
            std::ofstream cacheOutFile(m_CacheFile.string(), std::ios::binary);
            if (cacheOutFile) {
                int maxWrite = m_MaxAddress;
                cacheOutFile.write(reinterpret_cast<char*>(&maxWrite), sizeof (int));
                cacheOutFile.close();
                return true;
            }
        }
    }
    return false;
}
