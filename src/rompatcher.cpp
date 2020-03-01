#include "rompatcher.h"
#include "asar/asardll.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include "wrapper/filesystem.h"

bool istringcompare(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) {
        return false;
    }
    for (int i = 0; i < a.length(); i++) {
        char c = ::tolower(a[i]);
        if (c != ::tolower(b[i])) {
            return false;
        }
    }
    return true;
}

sable::RomPatcher::RomPatcher(
        const std::string& file,
        const std::string& name,
        const std::string& mode,
        int header
        ) : m_AState(AsarState::NotRun)
{
    if (!fs::exists(fs::path(file))) {
        throw std::runtime_error(fs::absolute(file).string() + " does not exist.");
    } else if (file.empty()) {
        throw std::logic_error("Filename is empty.");
    }
    if (istringcompare(mode, "lorom")) {
        m_MapType = Mapper::LOROM;
    } else if (istringcompare(mode, "exlorom")) {
        m_MapType = Mapper::EXLOROM;
    } else {
        m_MapType = Mapper::INVALID;
    }
    if (name.empty()) {
        m_Name = fs::path(file).stem().string();
    } else {
        m_Name = name;
    }
    std::ifstream inFile(file, std::ios::in|std::ios::binary|std::ios_base::ate);
    if (!inFile) {
        throw std::runtime_error(fs::absolute(file).string() + " could not be opened.");
    }
    auto pos = inFile.tellg();
    inFile.seekg(0, std::ios_base::beg);
    auto size = pos - inFile.tellg();
    if (header > 0) {
        m_HeaderSize = 512;
    } else if (header < 0) {
        m_HeaderSize = 0;
    } else {
        m_HeaderSize = size % 1024;
    }
    if (m_HeaderSize != 512 && m_HeaderSize != 0) {
        throw std::runtime_error(file + " has an invalid header.");
    }
    if ((size-m_HeaderSize) > ROM_MAX_SIZE) {
        throw std::runtime_error(file + " is too large.");
    }
    m_RomSize = size - m_HeaderSize;
    m_data.resize(size);
    inFile.read((char*)&m_data[0], size);

    inFile.close();
}

bool sable::RomPatcher::expand(int address)
{
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
    } else if (address <= m_RomSize) {
        return false;
    } else if (address >= NORMAL_ROM_MAX_SIZE) {
        std::cout << std::hex << address << '\n';
        if (address >= 6291456) {
            m_RomSize = ROM_MAX_SIZE;
        } else {
            m_RomSize = 6291456;
        }
    } else if (address < 131072) {
        m_RomSize = 131072;
    } else if (address < 262144) {
        m_RomSize = 262144;
    } else {
        m_RomSize = (1 + (address / 524288)) * 524288;
    }
    m_data.resize(m_HeaderSize + m_RomSize, 0);
    if (m_RomSize > NORMAL_ROM_MAX_SIZE) {
        auto internalHeader = m_data.begin()
                + util::ROMToPC(m_MapType, HEADER_LOCATION)
                + m_HeaderSize;
        m_MapType = util::getExpandedType(m_MapType);
        auto newHeader = m_data.begin()
                + util::ROMToPC(m_MapType, HEADER_LOCATION)
                + m_HeaderSize;
        std::copy(
                    internalHeader,
                    internalHeader + 0x20,
                    newHeader
                    );
    }
    return true;
}

bool sable::RomPatcher::applyPatchFile(const std::string &path, const std::string &format)
{
    std::vector<unsigned char> output(m_data);
    if (!fs::exists(path)) {
        throw std::logic_error("Could not open " + path + " patch file.");
    }
    if (format == "asm") {
        if (asar_init()) {
            if (asar_patch(path.c_str(), (char*)&m_data[m_HeaderSize], m_RomSize, &m_RomSize)) {
                m_AState = AsarState::Success;
            } else {
                m_AState = AsarState::Error;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
    return m_AState == AsarState::Success;
}

int sable::RomPatcher::getRomSize() const
{
    return m_RomSize;
}

int sable::RomPatcher::getRealSize() const
{
    return m_data.size();
}

unsigned char &sable::RomPatcher::at(int n)
{
    return m_data.at(n);
}

unsigned char &sable::RomPatcher::atROMAddr(int n)
{
    int addr = util::ROMToPC(m_MapType, n);
    if (addr == -1) {
        throw std::logic_error("Invalid SNES Address");
    }
    return m_data.at(addr + m_HeaderSize);
}

std::string sable::RomPatcher::getName() const
{
    return m_Name;
}

bool sable::RomPatcher::getMessages(std::back_insert_iterator<std::vector<std::string> > v)
{
    int count;
    if (m_AState == AsarState::Error) {
        auto* error = asar_geterrors(&count);
        for (int i = 0; i < count; i++){
            *(v++) = error[i].fullerrdata;
        }
    } else if (m_AState == AsarState::Success) {
        auto* prints = asar_getprints(&count);
        for (int i = 0; i < count; i++){
            *(v++) = prints[i];
        }
    } else {
        return false;
    }
    return true;

}
