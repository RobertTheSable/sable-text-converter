#ifndef ADDRESS_H
#define ADDRESS_H

#include <string>

namespace sable {
struct AddressNode {
    int address;
    std::string label;
    bool isTable;
};

struct TextNode {
    std::string files;
    size_t size;
    bool printpc;
};

}

#endif // ADDRESS_H
