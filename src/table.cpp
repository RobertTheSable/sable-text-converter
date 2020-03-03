#include "table.h"
#include "util.h"
#include <iomanip>

namespace sable {

Table::Table():
    m_AddressSize(3), m_StoreWidths(false), m_DataAddress(0)
{

}

Table::Table(int addressSize, bool storeWidths):
    m_AddressSize(addressSize), m_StoreWidths(storeWidths), m_DataAddress(0)
{

}

int Table::getAddressSize() const
{
    return m_AddressSize;
}

bool Table::getStoreWidths() const
{
    return m_StoreWidths;
}

void Table::addEntry(int address, int size)
{
    entries.push_back({"", address, size});
}

void Table::addEntry(const std::string &label)
{
    entries.push_back({label, -1, -1});
}

int Table::getSize() const
{
    int size = entries.size() * m_AddressSize;
    if (m_StoreWidths) {
        size *= 2;
    }
    return size;
}

std::vector<std::string> Table::getDataFromFile(std::istream &tablefile)
{
    std::vector<std::string> v;
    std::string input;
    int tableLine = 1;
    while (tablefile >> input) {
        std::string option;
        if (input == "address") {
            tablefile >> option;
            try {
                std::string::size_type sz;
                m_Address = std::stoi(option, &sz, 16);
                if (util::LoROMToPC(m_Address) == -1 || sz != option.length()) {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine)
                                + ": " + option + " is not a valid SNES address."
                                );
                }
            } catch (const std::invalid_argument& e) {
                throw std::runtime_error(
                            "line " + std::to_string(tableLine)
                            + ": " + option + " is not a valid SNES address."
                            );
            }
        } else if (input == "file") {
            if (tablefile >> option) {
                v.push_back(option);
            }
        } else if (input == "entry") {
            if (tablefile >> option) {
                if (option == "const") {
                    int size, address;
                    tablefile >> option;
                    if (option.back() == ',') {
                        option.pop_back();
                    }
                    address = util::strToHex(option).first;
                    if (getStoreWidths()) {
                        tablefile >> option;
                        size = util::strToHex(option).first;
                    } else {
                        size = 0;
                    }
                    addEntry(address, size);
                } else {
                    addEntry(option);
                }
            }
        } else if (input == "data") {
            if (tablefile >> option) {
                std::string::size_type sz;
                m_DataAddress = std::stoi(option, &sz, 16);
                if (util::LoROMToPC(m_DataAddress) == -1|| sz != option.length()) {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine)
                                + ": " + option + " is not a valid SNES address."
                                );
                }
            }
        } else if (input == "width") {
            if (tablefile >> option) {
                try {
                    int tableAddresssDataWidth = std::stoi(option);
                    if (tableAddresssDataWidth != 2 && tableAddresssDataWidth != 3) {
                        throw std::runtime_error(
                                    "Line " + std::to_string(tableLine) +
                                    ": width value should be 2 or 3."
                                    );
                    }
                    setAddressSize(tableAddresssDataWidth);
                } catch (const std::invalid_argument& e) {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine)
                                + ": width value should be 2 or 3."
                                );
                }

            }
        } else if (input == "savewidth") {
            setStoreWidths(true);
        }
        tableLine++;
    }
    if (m_DataAddress <= 0) {
        m_DataAddress = m_Address + getSize();
    }
    return v;
}

Table::iterator Table::begin()
{
    return iterator(entries.begin());
}

Table::iterator Table::end()
{
    return iterator(entries.end());
}

int Table::getAddress() const
{
    return m_Address;
}

void Table::setAddress(int Address)
{
    m_Address = Address;
}

int Table::getDataAddress() const
{
    return m_DataAddress;
}

void Table::setDataAddress(int DataAddress)
{
    m_DataAddress = DataAddress;
}

void Table::setAddressSize(int AddressSize)
{
    m_AddressSize = AddressSize;
}

void Table::setStoreWidths(bool StoreWidths)
{
    m_StoreWidths = StoreWidths;
}

bool operator!=(const Table::iterator &lsh, const Table::iterator &rhs)
{
    return lsh.m_position != rhs.m_position;
}

Table::iterator::iterator(const std::vector<Table::Entry>::iterator &position) : m_position(position)
{

}

Table::iterator &Table::iterator::operator++()
{
    ++m_position;
    return *this;
}

Table::Entry* Table::iterator::operator->()
{
    return &(*m_position);
}

Table::Entry Table::iterator::operator*()
{
    return *m_position;
}

}

