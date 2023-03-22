#include "datastore.h"
#include "exceptions.h"
#include "parse.h"
#include "util.h"

namespace sable {

DataStore::DataStore() : dirIndex(0), nextAddress(0), isSorted(false)
{

}

DataStore::DataStore(TextParser&& p)
    : m_Parser(std::move(p)), dirIndex(0), nextAddress(0), isSorted(false)
{
}

void DataStore::addFile(std::istream &input, const fs::path& path, std::ostream& errorStream, const sable::util::Mapper& mapper)
{
    if (nextAddress != 0 && mapper.ToPC(nextAddress) == -1) {
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
            std::tie(done, length) = m_Parser.parseLine(input, settings, std::back_inserter(tempFileData), mapper);
            line++;
        } catch (std::runtime_error &e) {
            throw ParseError("Error in text file " + path.string() + ", line " + std::to_string(line+1) + ": " + e.what());
        }
        if (settings.maxWidth > 0 && length > settings.maxWidth) {
            errorStream <<
                           "Warning in " + fs::absolute(path).string() + " on line " + std::to_string(line) +
                           ": Line is longer than the specified max width of " +
                           std::to_string(settings.maxWidth) + " pixels.\n";
        }
        if (done && !tempFileData.empty()) {
            if (m_Parser.getFonts()[settings.mode]) {
                if (settings.label.empty()) {
                    settings.label = dir + '_' + std::to_string(dirIndex++);
                }

                m_Addresses.push_back({settings.currentAddress, settings.label, false});
                // check for collisions
                {
                    int blockLocation = mapper.ToPC(settings.currentAddress);
                    if (auto result = textRanges.addBlock(
                            blockLocation,
                            blockLocation + (tempFileData.size() - lastSize),
                            settings.label,
                            fs::absolute(path).string()
                        ); result != Blocks::Collision::None) {
                        errorStream <<"Warning in " + fs::absolute(path).string() + ": block \"" + settings.label +"\" collides with block \"" +
                                      result->label + "\" from file \"" + result->file + "\".\n";
                    }
                } // collision check end

                { // add bianry file name and check for back crosses
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

                        int mirrorMask = 0;
                        if (mapper.getSize() <= util::NORMAL_ROM_MAX_SIZE) {
                            mirrorMask = (~settings.currentAddress & 0x800000);
                        }
                        settings.currentAddress = mapper.ToRom(mapper.ToPC(settings.currentAddress | 0xFFFF) +1) ^ mirrorMask;
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
                } // binary file add end

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
    util::Mapper m(util::MapperType::LOROM, false, true, 0x400000);
    try {
        files = table.getDataFromFile(tablefile, m);
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

const FontList &DataStore::getFonts() const
{
    return m_Parser.getFonts();
}

bool DataStore::getIsSorted() const
{
    return isSorted;
}
}
