#include "addresslist.h"
#include "exceptions.h"
#include "parse/parse.h"
#include "util.h"

namespace sable {

AddressList::AddressList() : nextAddress(0), isSorted(false)
{

}

std::vector<AddressNode>::const_iterator AddressList::begin() const
{
    return m_Addresses.begin();
}

std::vector<AddressNode>::const_iterator AddressList::end() const
{
    return m_Addresses.end();
}

void AddressList::addFile(const std::string &label, TextNode &&fileData)
{
     m_TextNodeList[label] = fileData;
}

void AddressList::addFile(const std::string &label, const std::string &file, std::size_t dataLength, bool printPC, options::ExportWidth exportWidth, options::ExportAddress exportAddress)
{
    addFile(label, {file, dataLength, printPC, exportWidth, exportAddress});
}

const TextNode &AddressList::getFile(const std::string &label) const
{
    if (m_TextNodeList.find(label) == m_TextNodeList.cend()) {
        throw ParseError("Label \"" + label + "\" not found.");
    }
    return m_TextNodeList.find(label)->second;
}

void AddressList::addTable(const std::string& name, Table &&tbl)
{
    m_Addresses.push_back({tbl.getAddress(), name, true});
    m_TableList[name] = tbl;
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

int AddressList::getNextAddress(const std::string& key) const
{
    if (key == "") {
        return nextAddress;
    } else if (auto result = m_TableList.find(key); result != m_TableList.end()) {
        auto res = result->second.getDataAddress();

        return res;
    }
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
}
