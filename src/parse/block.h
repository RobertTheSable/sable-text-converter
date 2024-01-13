#ifndef BLOCKPARSER_H
#define BLOCKPARSER_H

#include "data/textblockrange.h"

#include <string>
#include <vector>

namespace sable {

struct Block
{
    struct Bounds {
        int address;
        std::size_t start, length;
        std::string labelPrefix, fileSuffix;
        int getNextAddress() const;
    };

    std::vector<unsigned char> data;
    std::vector<Bounds> bankBounds;

    bool bankSplit() const;
    int getNextAddress() const;
    std::size_t length() const;

    Block(int currentAddress, int nextBankAddress, std::vector<unsigned char>&& data);
};

} // namespace sable


#endif // BLOCKPARSER_H
