#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>

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
    std::vector<std::string> getDataFromFile(std::istream& in);
    struct Entry
    {
        std::string label;
        int address;
        int size;
    };
    class iterator {
    public:
        iterator(const std::vector<Entry>::iterator& position);
        iterator &operator++();
        Entry* operator->();
        Entry operator*();
        friend bool operator!=(const iterator& lsh, const iterator& rhs);
    private:
        std::vector<Entry>::iterator m_position;
    };
    Table::iterator begin();
    Table::iterator end();

    int getAddress() const;
    void setAddress(int Address);

private:
    std::vector<Entry> entries;
    int m_AddressSize, m_Address, m_DataAddress;
    bool m_StoreWidths;
};
}


#endif // TABLE_H
