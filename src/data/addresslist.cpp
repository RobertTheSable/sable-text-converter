#include "addresslist.h"
#include "exceptions.h"
#include "parse/parse.h"
#include "util.h"

namespace sable {

AddressList::AddressList() : dirIndex(0), nextAddress(0), isSorted(false)
{

}

std::vector<std::string> AddressList::addTable(std::istream &tablefile, const fs::path &path)
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

void AddressList::addTable(const std::string& name, Table &&tbl)
{
    m_Addresses.push_back({tbl.getAddress(), name, true});
    m_TableList[name] = tbl;
}

std::vector<AddressList::AddressNode>::const_iterator AddressList::begin() const
{
    return m_Addresses.begin();
}

std::vector<AddressList::AddressNode>::const_iterator AddressList::end() const
{
    return m_Addresses.end();
}

void AddressList::addFile(const std::string &label, const std::string &file, std::size_t dataLength, bool printPC)
{
    m_TextNodeList[label] = {file, dataLength, printPC};
}

const AddressList::TextNode &AddressList::getFile(const std::string &label) const
{
    if (m_TextNodeList.find(label) == m_TextNodeList.cend()) {
        throw ParseError("Label \"" + label + "\" not found.");
    }
    return m_TextNodeList.find(label)->second;
}

const Table &AddressList::getTable(const std::string &label) const
{
    return m_TableList.at(label);
}

std::optional<Table> AddressList::checkTable(const std::string &label) const
{
    if (auto result = m_TableList.find(label); result != m_TableList.end()) {
        return result->second;
    }
    return std::nullopt;

}

void AddressList::sort()
{
    std::sort(m_Addresses.begin(), m_Addresses.end(), [] (AddressNode& lhs, AddressNode& rhs) {
        return lhs.address < rhs.address;
    });
    isSorted = true;
}

int AddressList::getNextAddress() const
{
    return nextAddress;
}

void AddressList::setNextAddress(int value)
{
    nextAddress = value;
}

void AddressList::addAddress(AddressNode n)
{
    m_Addresses.push_back(n);
}

FontList AddressList::getFonts() const
{
    return FontList{};
}

bool AddressList::getIsSorted() const
{
    return isSorted;
}
}
