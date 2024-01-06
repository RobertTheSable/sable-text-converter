#ifndef ADDRESS_H
#define ADDRESS_H

#include <string>

#include "options.h"

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
    options::ExportWidth exportWidth;
    options::ExportAddress exportAddress;
};

}

#endif // ADDRESS_H
