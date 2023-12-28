#ifndef DATASTORE_H
#define DATASTORE_H
#include <string>
#include <vector>
#include <iostream>
#include <stack>
#include <queue>
#include <optional>
#include "parse/parse.h"
#include "parse/fontlist.h"
#include "table.h"
#include "textblockrange.h"
#include "wrapper/filesystem.h"

namespace sable {

typedef std::back_insert_iterator<std::vector<unsigned char>> ByteInserter;

class AddressList
{
public:
    AddressList();
    struct TextNode {
        std::string files;
        size_t size;
        bool printpc;
    };
    struct AddressNode {
        int address;
        std::string label;
        bool isTable;
    };
    std::vector<AddressNode>::const_iterator begin() const;
    std::vector<AddressNode>::const_iterator end() const;

    void addFile(const std::string& label, const std::string& file, std::size_t dataLength, bool printPC);
    const TextNode& getFile(const std::string& label) const;

    std::vector<std::string> addTable(std::istream& file, const fs::path& path);
    void addTable(const std::string& name, Table&& tbl);
    const Table& getTable(const std::string& label) const;
    std::optional<Table> checkTable(const std::string& label) const;

    void sort();
    int getNextAddress() const;
    void setNextAddress(int value);
    void addAddress(AddressNode n);
    FontList getFonts() const;
    bool getIsSorted() const;


private:
//    TextParser m_Parser;
    std::vector<AddressNode> m_Addresses;
    std::unordered_map<std::string, TextNode> m_TextNodeList;
    std::unordered_map<std::string, Table> m_TableList;
    fs::path lastDir;
    int dirIndex;
    int nextAddress;
    bool isSorted;
    Blocks textRanges;
};
}

#endif // DATASTORE_H
