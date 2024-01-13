#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include "mapper.h"

namespace sable {
class Table {
public:
    Table();
    Table(int addressSize, bool storeWidths);
    int getAddressSize() const;
    bool getStoreWidths() const;
    void setAddressSize(int addressSize);
    void setStoreWidths(bool storeWidths);
    void addEntry(int address, int size);
    void addEntry(const std::string& label);
    int getDataAddress() const;
    void setDataAddress(int DataAddress);
    int getSize() const;
    size_t getEntryCount() const;
    std::vector<std::string> getDataFromFile(std::istream& in, const util::Mapper& mapper);
    struct Entry
    {
        std::string label;
        int address;
        int size;
    };
    class iterator {
    public:
        iterator(const std::vector<Entry>::const_iterator& position);
        iterator &operator++();
        const Entry* operator->() const;
        const Entry& operator*() const;
        friend bool operator!=(const iterator& lsh, const iterator& rhs);
    private:
        std::vector<Entry>::const_iterator m_position;
    };
    Table::iterator begin() const;
    Table::iterator end() const;

    int getAddress() const;
    void setAddress(int Address);

private:
    std::vector<Entry> entries;
    int m_AddressSize, m_Address, m_DataAddress;
    bool m_StoreWidths;
};
}


#endif // TABLE_H
