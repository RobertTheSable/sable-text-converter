#include "datastore.h"
#include "exceptions.h"
#include "parse.h"
#include "util.h"

namespace sable {

DataStore::DataStore() : nextAddress(0), dirIndex(0), isSorted(false), m_RomType(util::Mapper::INVALID)
{

}

DataStore::DataStore(const YAML::Node &config, const std::string& defaultMode, util::Mapper mapType)
    : m_Parser(config, defaultMode), nextAddress(0), dirIndex(0), isSorted(false), m_RomType(mapType)
{
}

void DataStore::addFile(std::istream &input, const fs::path& path, std::ostream& errorStream)
{
    if (nextAddress != 0 && util::ROMToPC(m_RomType, nextAddress) == -1) {
        std::ostringstream error;
        error << "Attempted to begin parsing with invalid ROM address $" << std::hex << nextAddress;
        throw ParseError(error.str());
    }
    if (!input) {
        throw std::logic_error("Could not read data from stream.");
    }
    tempFileData.clear();
    std::string dir = path.parent_path().filename().string();
    if (dir != lastDir.string()) {
        if (m_TableList.find(dir) != m_TableList.end()) {
            nextAddress = m_TableList.at(dir).getDataAddress();
        }
        dirIndex = 0;
    }
    lastDir = path.parent_path();
    ParseSettings settings =  m_Parser.getDefaultSetting(nextAddress);
    int line = 0;
    size_t lastSize = 0;
    while (input.peek() != EOF) {
        bool done;
        int length;
        try {
            std::tie(done, length) = m_Parser.parseLine(input, settings, std::back_inserter(tempFileData));
            line++;
        } catch (std::runtime_error &e) {
            throw ParseError("Error in text file " + path.string() + ": " + e.what());
        }
        if (settings.maxWidth > 0 && length > settings.maxWidth) {
            errorStream <<
                           "Warning in " + fs::absolute(path).string() + " on line " + std::to_string(line) +
                           ": Line is longer than the specified max width of " +
                           std::to_string(settings.maxWidth) + " pixels.\n";
        }
        if (done && !tempFileData.empty()) {
            if (m_Parser.getFonts().at(settings.mode)) {
                if (settings.label.empty()) {
                    settings.label = dir + '_' + std::to_string(dirIndex++);
                }

                m_Addresses.push_back({settings.currentAddress, settings.label, false});
                {
                    int tmpAddress = settings.currentAddress + tempFileData.size();
                    size_t dataLength;
                    bool printpc = settings.printpc;
                    int bankDifference = ((tmpAddress & 0xFF0000) - (settings.currentAddress & 0xFF0000)) >>16;
                    if (bankDifference > 0) {
                        if (bankDifference > 1) {
                            throw ParseError(path.string() + ", line " + std::to_string(line) +": read data that would cross two banks, aborting.");
                        }
                        size_t bankLength = ((settings.currentAddress + tempFileData.size()) & 0xFFFF);
                        dataLength = (tempFileData.size() - lastSize) - bankLength;
                        binaryOutputStack.push({settings.label + ".bin", dataLength});
                        binaryOutputStack.push({settings.label + "bank.bin", bankLength});

                        bool highType = (settings.currentAddress & 0x800000) == 0;
                        settings.currentAddress = util::PCToROM(m_RomType, util::ROMToPC(m_RomType, settings.currentAddress | 0xFFFF) +1, highType);
                        m_Addresses.push_back({settings.currentAddress, "$" + settings.label, false});
                        settings.currentAddress += bankLength;
                        m_TextNodeList["$" + settings.label] = {
                            settings.label + "bank.bin",
                            bankLength,
                            printpc
                        };
                        printpc = false;
                    } else {
                        dataLength = tempFileData.size() - lastSize;
                        settings.currentAddress += dataLength;
                        binaryOutputStack.push({settings.label + ".bin", dataLength});
                    }
                    m_TextNodeList[settings.label] = {
                        settings.label + ".bin", dataLength, printpc
                    };
                }
                settings.label = "";
                settings.printpc = false;
            }
            lastSize = tempFileData.size();
        }
    }
    nextAddress = settings.currentAddress;
    if (settings.maxWidth < 0) {
        settings.maxWidth = 0;
    }
}

std::pair<std::string, int> DataStore::getOutputFile()
{
    std::pair<std::string, int> returnVal = std::make_pair("", 0);
    if (!binaryOutputStack.empty()) {
        returnVal = binaryOutputStack.front();
        binaryOutputStack.pop();
    }
    return returnVal;
}

std::vector<std::string> DataStore::addTable(std::istream &tablefile, const fs::path &path)
{
    std::vector<std::string> files;
    fs::path dir(fs::path(path).parent_path());
    std::string label = dir.filename().string();
    Table table;
    table.setAddress(nextAddress);
    try {
        files = table.getDataFromFile(tablefile);
    } catch (std::runtime_error &e) {
        throw ParseError("Error in table " + path.string() + ", " + e.what());
    }
    m_TableList[label] = table;
    m_Addresses.push_back({table.getAddress(), label, true});

    return files;
}

std::vector<DataStore::AddressNode>::const_iterator DataStore::begin() const
{
    return m_Addresses.begin();
}

std::vector<DataStore::AddressNode>::const_iterator DataStore::end() const
{
    return m_Addresses.end();
}

std::vector<unsigned char>::const_iterator DataStore::data_begin()
{
    return tempFileData.cbegin();
}

std::vector<unsigned char>::const_iterator DataStore::data_end()
{
    return tempFileData.cend();
}

const DataStore::TextNode &DataStore::getFile(const std::string &label) const
{
    if (m_TextNodeList.find(label) == m_TextNodeList.cend()) {
        throw ParseError("Label \"" + label + "\" not found.");
    }
    return m_TextNodeList.find(label)->second;
}

const Table &DataStore::getTable(const std::string &label) const
{
    return m_TableList.at(label);
}

void DataStore::sort()
{
    std::sort(m_Addresses.begin(), m_Addresses.end(), [] (AddressNode& lhs, AddressNode& rhs) {
        return lhs.address < rhs.address;
    });
    isSorted = true;
}

int DataStore::getNextAddress() const
{
    return nextAddress;
}

void DataStore::setNextAddress(int value)
{
    nextAddress = value;
}

const std::map<std::string, Font> &DataStore::getFonts() const
{
    return m_Parser.getFonts();
}

bool DataStore::getIsSorted() const
{
    return isSorted;
}
}
