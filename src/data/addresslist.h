#ifndef DATASTORE_H
#define DATASTORE_H
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include "table.h"
#include "textblockrange.h"
#include "address.h"

namespace sable {

class AddressList
{
public:
    AddressList();
    std::vector<AddressNode>::const_iterator begin() const;
    std::vector<AddressNode>::const_iterator end() const;


    void addFile(const std::string& label, TextNode&& fileData);
    void addFile(
        const std::string& label,
        const std::string& file,
        std::size_t dataLength,
        bool printPC,
        options::ExportWidth exportWidth,
        options::ExportAddress exportAddress
    );
    const TextNode& getFile(const std::string& label) const;

    void addTable(const std::string& name, Table&& tbl);
    const Table& getTable(const std::string& label) const;
    std::optional<Table> checkTable(const std::string& label) const;

    void sort();
    int getNextAddress(const std::string& key) const;
    void setNextAddress(int value);
    void addAddress(AddressNode n);

    template<class ...Args>
    void addAddress(int address, Args&& ...args)
    {
        addAddress(AddressNode{address, std::forward<Args>(args)...});
    }

private:
    std::vector<AddressNode> m_Addresses;
    std::unordered_map<std::string, TextNode> m_TextNodeList;
    std::unordered_map<std::string, Table> m_TableList;
    int nextAddress;
    bool isSorted;
    Blocks textRanges;
};
}

#endif // DATASTORE_H
