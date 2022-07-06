#ifndef DATASTORE_H
#define DATASTORE_H
#include <string>
#include <vector>
#include <iostream>
#include <stack>
#include <queue>
#include "parse.h"
#include "table.h"
#include "wrapper/filesystem.h"

namespace sable {

typedef std::back_insert_iterator<std::vector<unsigned char>> ByteInserter;

class DataStore
{
public:
    DataStore();
    DataStore(TextParser&& p);
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
    void addFile(std::istream& file, const fs::path& path, std::ostream& errorStream, const sable::util::Mapper& mapper);
    std::pair<std::string, int> getOutputFile();
    std::vector<std::string> addTable(std::istream& file, const fs::path& path);
    std::vector<AddressNode>::const_iterator begin() const;
    std::vector<AddressNode>::const_iterator end() const;
    std::vector<unsigned char>::const_iterator data_begin();
    std::vector<unsigned char>::const_iterator data_end();
    const TextNode& getFile(const std::string& label) const;
    const Table& getTable(const std::string& label) const;
    void sort();
    int getNextAddress() const;
    void setNextAddress(int value);
    const FontList& getFonts() const;
    bool getIsSorted() const;

private:
    TextParser m_Parser;
    std::vector<AddressNode> m_Addresses;
    std::unordered_map<std::string, TextNode> m_TextNodeList;
    std::unordered_map<std::string, Table> m_TableList;
    std::queue <std::pair<std::string, int>> binaryOutputStack;
    std::vector<unsigned char> tempFileData;
    fs::path lastDir;
    int dirIndex;
    int nextAddress;
    bool isSorted;
};
}

#endif // DATASTORE_H
