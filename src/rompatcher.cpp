#include "rompatcher.h"
#include "asar/asardll.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <bitset>
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

sable::RomPatcher::RomPatcher(const std::string &mode) : m_AState(AsarState::NotRun)
{
    if (istringcompare(mode, "lorom")) {
        m_MapType = Mapper::LOROM;
    } else if (istringcompare(mode, "exlorom")) {
        m_MapType = Mapper::EXLOROM;
    } else {
        m_MapType = Mapper::INVALID;
    }
}

void sable::RomPatcher::loadRom(const std::string &file, const std::string &name, int header)
{
    if (!fs::exists(fs::path(file))) {
        throw std::runtime_error(fs::absolute(file).string() + " does not exist.");
    } else if (file.empty()) {
        throw std::logic_error("Filename is empty.");
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

unsigned char &sable::RomPatcher::at(int n)
{
    return m_data.at(n);
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

int sable::RomPatcher::getRealSize() const
{
    return m_data.size();
}

void sable::RomPatcher::writeParsedData(const sable::DataStore &m_DataStore, const fs::path& includePath, std::ostream &mainText, std::ostream &textDefines)
{
    for (auto& node: m_DataStore) {
        //int dif = it.address - lastPosition;
        if (node.isTable) {
            textDefines << generateAssignment("def_table_" + node.label, node.address, 3) << '\n';
            mainText  << "ORG " + generateDefine("def_table_" + node.label) + '\n';
            const Table& t = m_DataStore.getTable(node.label);
            mainText << "table_" + node.label + ":\n";
            for (auto it : t) {
                int size;
                std::string dataType;
                if (t.getAddressSize() == 3) {
                    dataType = "dl";
                } else if(t.getAddressSize() == 2) {
                    dataType = "dw";
                } else {
                    throw std::logic_error("Unsupported address size " + std::to_string(t.getAddressSize()));
                }
                if (it.address > 0) {
                    mainText << dataType + ' ' + generateNumber(it.address, 2);
                    size = it.size;
                } else {
                    mainText << dataType + ' ' + it.label;
                    size = m_DataStore.getFile(it.label).size;
                }
                if (t.getStoreWidths()) {
                    mainText << ", " << std::dec << size;
                }
                mainText << '\n';
            }
        } else {
            if (node.label.front() == '$') {
                mainText  << "ORG " + generateNumber(node.address, 3) << '\n';
            } else {
                textDefines << generateAssignment("def_" + node.label, node.address, 3) << '\n';
                mainText  << "ORG " + generateDefine("def_" + node.label) + '\n'
                          << node.label + ":\n";
            }
            std::string token = m_DataStore.getFile(node.label).files;
            mainText << generateInclude(includePath / token , fs::path(), true) + '\n';
                    // << "incbin " + (fs::path(m_BinsDir) / m_TextOutDir / token).string() + '\n';
            if (m_DataStore.getFile(node.label).printpc) {
                mainText << "print pc\n";
            }
        }
        mainText << '\n';
    }
}

void sable::RomPatcher::writeIncludes(sable::ConstStringIterator start, sable::ConstStringIterator end, std::ostream &mainFile, const fs::path& includePath)
{
    for (auto it = start; it != end; ++it) {
        mainFile << generateInclude(includePath / *it, fs::path(), false) << '\n';
    }
}

void sable::RomPatcher::writeFontData(const sable::DataStore &data, std::ofstream &output)
{
    for (auto& fontIt: data.getFonts()) {
        if (!fontIt.second.getFontWidthLocation().empty()) {
            output << "\n"
                      "ORG " + fontIt.second.getFontWidthLocation();
            std::vector<int> widths;
            widths.reserve(fontIt.second.getMaxEncodedValue());
            fontIt.second.getFontWidths(std::back_insert_iterator(widths));
            int column = 0;
            int skipCount = 0;
            for (auto it = widths.begin(); it != widths.end(); ++it) {
                int width = *it;
                if (width == 0) {
                    skipCount++;
                    column = 0;
                } else {
                    if (skipCount > 0) {
                        output << "\n"
                                  "skip " << std::dec << skipCount;
                        skipCount = 0;
                    }
                    if (column == 0) {
                        output << "\ndb ";
                    } else {
                        output << ", ";
                    }
                    output << "$" << std::hex << std::setw(2) << std::setfill('0') << width;
                    column++;
                    if (column ==16) {
                        column = 0;
                    }
                }
            }
            output << '\n' ;
        }
    }
}

std::string sable::RomPatcher::generateInclude(const fs::path &file, const fs::path &basePath, bool isBin) const
{
    std::string includePath;
    if (isBin) {
        includePath = "incbin ";
    } else {
        includePath = "incsrc ";
    }
    if (basePath.empty() || file.string().find_first_of(basePath.string()) != 0) {
        includePath += file.generic_string();
    } else {
        // can't use fs::relative because g++ 7 doesn't support it.
        includePath +=  file.generic_string().substr(basePath.generic_string().length() + 1);
    }
    return includePath;
}

std::string sable::RomPatcher::generateDefine(const std::string& label) const
{
    return std::string("!") + label;
}

std::string sable::RomPatcher::generateNumber(int number, int width, int base) const
{
    std::ostringstream output;
    if (base == 16) {
        if (width > 4 || width <= 0) {
            // Asar does support 4-byte data even though the SNES doesn't really.
            throw std::runtime_error(std::string("Unsupported width ") +  std::to_string(width));
        }
        output << '$' << std::setfill('0') << std::setw(width *2) << std::hex << number;
    } else if (base == 10) {
        output << std::dec << number;
    } else if (base == 2) {
        if (width == 4) {
            output << '%' + std::bitset<32>(number).to_string();
        } else if (width == 3) {
            output << '%' + std::bitset<24>(number).to_string();
        } else if (width == 2) {
            output << '%' + std::bitset<16>(number).to_string();
        } else if (width == 1) {
            output << '%' + std::bitset<8>(number).to_string();
        } else {
            throw std::runtime_error(std::string("Unsupported width ") +  std::to_string(width));
        }

    } else {
        throw std::runtime_error(std::string("Unsupported base ") +  std::to_string(base));
    }
    return output.str();

}

std::string sable::RomPatcher::generateAssignment(const std::string& label, int value, int width, int base, const std::string& baseLabel) const
{
    std::ostringstream output;
    output << generateDefine(label) <<  " = ";
    if (!baseLabel.empty()) {
        output << generateDefine(baseLabel);
        if (value != 0) {
            output << '+' + generateNumber(value, width, base);
        }
    } else {
        output << generateNumber(value, width, base);
    }


    return output.str();
}

//int sable::RomPatcher::getRomSize() const
//{
//    return m_RomSize;
//}

//unsigned char &sable::RomPatcher::atROMAddr(int n)
//{
//    int addr = util::ROMToPC(m_MapType, n);
//    if (addr == -1) {
//        throw std::logic_error("Invalid SNES Address");
//    }
//    return m_data.at(addr + m_HeaderSize);
//}

//std::string sable::RomPatcher::getName() const
//{
//    return m_Name;
//}

