#include "table.h"
#include "util.h"
#include <iomanip>
#include <algorithm>

namespace sable {

Table::Table():
    m_AddressSize(3), m_StoreWidths(false), m_DataAddress(0), m_Address(0)
{

}

Table::Table(int addressSize, bool storeWidths):
    m_AddressSize(addressSize), m_StoreWidths(storeWidths), m_DataAddress(0), m_Address(0)
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
    entries.push_back({std::string(""), address, size});
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

size_t Table::getEntryCount() const
{
    return entries.size();
}

std::vector<std::string> Table::getDataFromFile(std::istream &tableFile)
{
    std::vector<std::string> v;
    std::string line, input;
    int tableLine = 1;
    while (!getline(tableFile, line).fail()) {
        if (line.find('\r') != std::string::npos) {
            line.erase(line.find('\r'), 1);
        }
        if (!line.empty()) {
            std::istringstream lineStream(line);
            lineStream >> input;
            std::string option;
            if (input == "address") {
                if (!(lineStream >> option)) {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine) +
                                ": missing value for table address."
                                );
                }
                auto result = util::strToHex(option);
                if (result.second < 0 || util::LoROMToPC(result.first) == -1) {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine) +
                                ": " + option + " is not a valid SNES address."
                                );
                }
                m_Address = result.first;
            } else if (input == "file") {
                if (lineStream >> option) {
                    v.push_back(option);
                } else {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine) +
                                ": missing filename."
                                );
                }
            } else if (input == "entry") {
                if (lineStream >> option) {
                    if (option == "const") {
                        int size, address;
                        if (!(lineStream >> option)) {
                            throw std::runtime_error(
                                        "line " + std::to_string(tableLine) +
                                        ": missing data for constant entry."
                                        );
                        }
                        if (option.back() == ',') {
                            // Probably should cause an error here
                            // if (!getStoreWidths()) {}
                            option.pop_back();
                        }
                        auto result = util::strToHex(option);
                        if (result.second < 0) {
                            throw std::runtime_error(
                                        "line " + std::to_string(tableLine) +
                                        ": " + option + " is not a valid SNES address."
                                        );
                        } else if (result.second > m_AddressSize) {
                            throw std::runtime_error(
                                        "line " + std::to_string(tableLine) +
                                        ": " + option + " is larger than the max width for the table."
                                        );
                        }
                        address = result.first;
                        if (getStoreWidths()) {
                            if (!(lineStream >> option)) {
                                throw std::runtime_error(
                                            "line " + std::to_string(tableLine) +
                                            ": missing width for constant entry."
                                            );
                            }
                            if (option.front() == '$') {
                                result = util::strToHex(option);
                                if (result.second < 0) {
                                    throw std::runtime_error(
                                                "line " + std::to_string(tableLine) +
                                                ": " + option + " is not a valid hexadecimal number."
                                                );
                                }
                                size = result.first;
                            } else {
                                try {
                                    size = std::stoi(option);
                                } catch (std::invalid_argument &e) {
                                    throw std::runtime_error(
                                                "line " + std::to_string(tableLine) +
                                                ": " + option + " is not a decimal or hex number."
                                                );
                                }
                            }
                        } else {
                            // Probably should cause an error if widths aren't being stored.
                            // if (!lineStream.eof())
                            size = -1;
                        }
                        addEntry(address, size);
                    } else {
                        addEntry(option);
                    }
                } else {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine) +
                                ": missing data for table entry."
                                );
                }
            } else if (input == "data") {
                if (lineStream >> option) {
                    auto result = util::strToHex(option);
                    if (result.second < 0 || util::LoROMToPC(result.first) == -1) {
                        throw std::runtime_error(
                                    "line " + std::to_string(tableLine) +
                                    ": " + option + " is not a valid SNES address."
                                    );
                    }
                    m_DataAddress = result.first;
                } else {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine) +
                                ": missing address for table data address setting."
                                );
                }
            } else if (input == "width") {
                if (lineStream >> option) {
                    try {
                        int tableAddresssDataWidth = std::stoi(option);
                        if (tableAddresssDataWidth != 2 && tableAddresssDataWidth != 3) {
                            throw std::runtime_error(
                                        "line " + std::to_string(tableLine) +
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

                } else {
                    throw std::runtime_error(
                                "line " + std::to_string(tableLine) +
                                ": missing value for table width setting."
                                );
                }
            } else if (input == "savewidth") {
                // Probably should cause an error if widths aren't being stored.
                // if (!lineStream.eof())
                setStoreWidths(true);
            } else {
                throw std::runtime_error(
                            "line " + std::to_string(tableLine)
                            + ": unrecognized setting \"" + input + "\""
                            );
            }
        }
        tableLine++;
    }
    if (m_DataAddress <= 0) {
        m_DataAddress = m_Address + getSize();
    }
    return v;
}

Table::iterator Table::begin() const
{
    return iterator(entries.cbegin());
}

Table::iterator Table::end() const
{
    return iterator(entries.cend());
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

Table::iterator::iterator(const std::vector<Table::Entry>::const_iterator &position) : m_position(position)
{

}

Table::iterator &Table::iterator::operator++()
{
    ++m_position;
    return *this;
}

const Table::Entry* Table::iterator::operator->() const
{
    return &(*m_position);
}

const Table::Entry& Table::iterator::operator*() const
{
    return *m_position;
}

}

