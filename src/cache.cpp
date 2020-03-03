#include "cache.h"
#include <fstream>

sable::Cache::Cache(const std::string& path) : m_MaxAddress(0), m_IsReadable(false)
{
    if (path.empty()) {
        m_CacheFile = fs::path("cache") / "cache.bin";
    } else {
        m_CacheFile = fs::path(path) / "cache" / "cache.bin";
    }

    if (fs::exists(m_CacheFile)) {
        std::ifstream input(m_CacheFile.string(), std::ios::binary);
        input.read(reinterpret_cast<char*>(&m_MaxAddress), sizeof (int));
        input.close();
        m_IsReadable = true;
    }
}

int sable::Cache::getMaxAddress() const
{
    if (m_IsReadable) {
        return m_MaxAddress;
    } else {
        return 0;
    }
}

void sable::Cache::setMaxAddress(int value)
{
    m_MaxAddress = value;
    m_IsReadable = true;
}

bool sable::Cache::isReadable() const
{
    return m_IsReadable;
}

bool sable::Cache::write() const
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
