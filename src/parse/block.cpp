#include "block.h"

#include <stdexcept>

int sable::Block::Bounds::getNextAddress() const
{
    return address + length;
}

bool sable::Block::bankSplit() const
{
    return bankBounds.size() > 1;
}

int sable::Block::getNextAddress() const
{
    return bankBounds.front().getNextAddress();
}

std::size_t sable::Block::length() const
{
    return data.size();
}

sable::Block::Block(int currentAddress, int nextBankAddress, std::vector<unsigned char> &&data_): data(data_)
{
    auto blockLength =  data.size();
    auto blockEndAddress = currentAddress + blockLength;

    std::size_t dataLength = 0;
    int bankDifference = ((blockEndAddress & 0xFF0000) - (currentAddress & 0xFF0000)) >>16;
    int dataStart = 0;
    auto insertPos = bankBounds.end();
    int firstAddress = currentAddress;
    if (bankDifference > 0) {
        size_t bankLength = ((currentAddress + blockLength) & 0xFFFF);
        dataLength = blockLength - bankLength;

        blockEndAddress = nextBankAddress + bankLength;
        bankDifference = ((blockEndAddress & 0xFF0000) - (nextBankAddress & 0xFF0000)) >>16;
        if (bankDifference > 0) {
            throw std::runtime_error("read data that would cross two banks, aborting.");
        }

        bankBounds.push_back(
            Bounds{
                nextBankAddress,
                dataLength,
                bankLength,
                "$",
                "bank"
            }
        );

        // possibly a bug
        // but it shoudln't matter since the addresses will get sorted after all files are processed
        insertPos = bankBounds.begin() + 1;
    } else {
        dataLength = blockLength;
    }

    bankBounds.insert(
        insertPos,
        Bounds{
            firstAddress,
            0,
            dataLength,
            "",
            ""
        }
    );
}
