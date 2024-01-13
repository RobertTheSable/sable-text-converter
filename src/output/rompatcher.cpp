#include "rompatcher.h"
#include "asar/asardll.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <bitset>
#include <cmath>
#include "wrapper/filesystem.h"

#include "data/addresslist.h"
#include "data/optionhelpers.h"

#include "outputcapture.h"
#include "formatter.h"


bool sable::RomPatcher::succeeded(AsarState state)
{
    return state == AsarState::Success;
}

bool sable::RomPatcher::wasRun(AsarState state)
{
    return (state == AsarState::Success) || (state == AsarState::Error);
}

sable::RomPatcher::RomPatcher(const util::MapperType& mapper)
    : m_MapType(mapper), m_AState(AsarState::NotRun)
{

}

sable::RomPatcher::~RomPatcher()
{
    if (m_AState != AsarState::NotRun) {
        asar_close();
    }
}

bool sable::RomPatcher::loadRom(const std::string &file, const std::string &name, int header)
{
    if (!fs::exists(fs::path(file))) {
        return false;
    } else if (file.empty()) {
        throw std::logic_error("Filename is empty.");
    }

    std::ifstream inFile(file, std::ios::in|std::ios::binary|std::ios_base::ate);
    if (!inFile) {
        throw std::runtime_error(fs::absolute(file).string() + " could not be opened.");
    }
    auto pos = inFile.tellg();
    inFile.seekg(0, std::ios_base::beg);
    unsigned long size = pos - inFile.tellg();
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
    if ((size-m_HeaderSize) >= util::MAX_ALLOWED_FILESIZE_SHORTCUT) {
        throw std::runtime_error(file + " is too large.");
    }
    if ((size-m_HeaderSize) <= util::NORMAL_ROM_MAX_SIZE) {
        switch (m_MapType) {
            case util::MapperType::EXHIROM:
                m_MapType = util::MapperType::HIROM;
                break;
            case util::MapperType::EXLOROM:
                m_MapType = util::MapperType::LOROM;
                break;
            default:;
        }
    }
    m_RomSize = size - m_HeaderSize;
    m_data.resize(size);
    inFile.read((char*)&m_data[0], size);

    inFile.close();
    return true;
}

void sable::RomPatcher::clear()
{
    m_data.clear();
}

bool sable::RomPatcher::expand(int size, const util::Mapper& mapper)
{
    if (size <= 0 || size > util::MAX_ALLOWED_FILESIZE_SHORTCUT) {
        throw std::runtime_error("Invalid rom size.");
    }
    if (util::getExpandedType(m_MapType) != mapper.getType() && m_MapType != mapper.getType()) {
        throw std::logic_error("Tried to expand ROM with incompatible mapper types");
    }
    auto oldsize = m_RomSize;
    m_RomSize = size;
    m_data.resize(m_HeaderSize + m_RomSize, 0);
    util::Mapper oldMapper(m_MapType, m_HeaderSize != 0, true, util::NORMAL_ROM_MAX_SIZE);
    if (oldsize <= util::NORMAL_ROM_MAX_SIZE && m_RomSize > util::NORMAL_ROM_MAX_SIZE) {
        auto oldHeader = m_data.begin()
                + oldMapper.ToPC(util::HEADER_LOCATION);
        //auto oldMapType = m_MapType;
        //m_MapType = util::getExpandedType(m_MapType);
        auto newHeader = m_data.begin()
                + mapper.ToPC(util::HEADER_LOCATION)
                + m_HeaderSize;
        // TODO: check rom name or dev ID to see if extended header needs to be copied
        int startAddress = 0, stopAddress = 0, modeMask = 0;
        if (mapper.getType() == util::MapperType::EXHIROM) {
            startAddress = 0x408000;
            stopAddress = 0x410000;
            modeMask = 4;
        } else if (mapper.getType() == util::MapperType::EXLOROM) {
            startAddress = 0x008000;
            stopAddress = 0x018000;
            modeMask = 0;
        } else {
            throw std::logic_error("Tried to expand with a non-expanded mapper.");
        }
        std::copy(
            m_data.begin()+(oldMapper.ToPC(startAddress)),
            m_data.begin()+(oldMapper.ToPC(stopAddress)),
            m_data.begin()+(m_HeaderSize + mapper.ToPC(startAddress))
        );
        // Update the internal size and mapper mode, if needed
        *(oldHeader+0x17) = ceil(log((int)(m_RomSize/1024)) / log(2));
        *(newHeader+0x17) = ceil(log((int)(m_RomSize/1024)) / log(2));
        *(oldHeader+0x15) |= modeMask;
        *(newHeader+0x15) |= modeMask;
    }
    return true;
}

auto sable::RomPatcher::applyPatchFile(const std::string &path, const std::string &format) -> AsarState
{
    std::vector<unsigned char> output(m_data);
    if (!fs::exists(path)) {
        throw std::logic_error("Could not open " + path + " patch file.");
    }

    std::ostringstream sink;
    OutputCapture buffer{sink};
    if (format == "asm") {
        if (asar_init()) {
            if (asar_patch(path.c_str(), (char*)&m_data[m_HeaderSize], m_RomSize, &m_RomSize)) {
                m_AState = AsarState::Success;
            } else {
                m_AState = AsarState::Error;
            }
        } else {
            m_AState = AsarState::InitFailed;
            buffer.write();
            buffer.flush();
            throw std::runtime_error(std::string{"Failed to initialize Asar library: "} + sink.str());
        }
    } else {
        m_AState = AsarState::Error;
    }

    return m_AState;
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

void sable::RomPatcher::writeParsedData(const sable::AddressList &addresses, const fs::path& includePath, std::ostream &mainText, std::ostream &textDefines)
{
    int predictedNextAddress = 0;
    for (auto& node: addresses) {
        if (node.isTable) {
            textDefines << formatter::generateAssignment("def_table_" + node.label, node.address, 3) << '\n';
            mainText  << "ORG " + formatter::generateDefine("def_table_" + node.label) + '\n';
            const Table& t = addresses.getTable(node.label);
            mainText << "table_" + node.label + ":\n";
            for (auto it : t) {
                int size = 0;
                std::string dataType;
                if (t.getAddressSize() == 3) {
                    dataType = "dl";
                } else if(t.getAddressSize() == 2) {
                    dataType = "dw";
                } else {
                    throw std::logic_error("Unsupported address size " + std::to_string(t.getAddressSize()));
                }
                if (it.address > 0) {
                    mainText << dataType + ' ' + formatter::generateNumber(it.address, t.getAddressSize());
                    size = it.size;
                } else {
                    mainText << dataType + ' ' + it.label;
                    size = addresses.getFile(it.label).size;
                }
                if (t.getStoreWidths()) {
                    mainText << ", " << std::dec << size;
                }
                mainText << '\n';
            }
        } else {
            auto file = addresses.getFile(node.label);
            if (node.label.front() == '$') {
                mainText  << "ORG " + formatter::generateNumber(node.address, 3) << '\n';
            } else {
                if (predictedNextAddress == 0 ||
                    predictedNextAddress != node.address ||
                    options::isEnabled(file.exportAddress)
                ) {
                    textDefines << formatter::generateAssignment("def_" + node.label, node.address, 3) + '\n';
                    mainText  << "ORG " + formatter::generateDefine("def_" + node.label) + '\n';
                }
                if (options::isEnabled(file.exportWidth)) {
                    textDefines <<
                        formatter::generateAssignment(
                            "def_" + node.label + "_length",
                            file.size,
                            3,
                            10
                        ) + '\n'
                    ;
                }
                mainText << node.label + ":\n";
            }

            mainText << formatter::generateInclude(includePath / file.files , fs::path(), true) + '\n';
            if (file.printpc) {
                mainText << "print pc\n";
            }
            predictedNextAddress = node.address + file.size;
        }
        mainText << '\n';
    }
}

void sable::RomPatcher::writeInclude(const std::string include, std::ostream &mainFile, const fs::path &includePath)
{
    mainFile << formatter::generateInclude(includePath / include, fs::path(), false) << '\n';
}

void sable::RomPatcher::writeIncludes(sable::ConstStringIterator start, sable::ConstStringIterator end, std::ostream &mainFile, const fs::path& includePath)
{
    for (auto it = start; it != end; ++it) {
        mainFile << formatter::generateInclude(includePath / *it, fs::path(), false) << '\n';
    }
}

std::string sable::RomPatcher::getMapperDirective(const util::MapperType& mapper)
{
    std::string value;
    switch (mapper) {
        case util::MapperType::LOROM:
            value = "lorom";
            break;
        case util::MapperType::EXLOROM:
            value = "exlorom";
            break;
        case util::MapperType::HIROM:
            value = "hirom";
            break;
        case util::MapperType::EXHIROM:
            value = "exhirom";
            break;
        default:
            throw std::logic_error("Invalid map type.");
    }
    return value;
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

