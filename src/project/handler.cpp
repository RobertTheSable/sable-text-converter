#include "groupparser.h"

namespace sable {

void outputFile(const std::string &file, const std::vector<unsigned char>& data, size_t length, size_t start)
{
    std::ofstream output(
        file,
        std::ios::binary | std::ios::out
    );
    if (!output) {
        throw ASMError("Could not open " + file + " for writing");
    }
    output.write(reinterpret_cast<const char*>(&data[start]), length);
    output.close();
}

void Handler::report(std::string file, error::Levels l, std::string msg, int line)
{
        // probably this can be replaced with std::format whenever I get reliable access to it
        switch (l) {
        case error::Levels::Error:
            throw ParseError("Error in text file " + file + ", line " + std::to_string(line) + ": " + msg);
            break;
        case error::Levels::Warning:
            output <<
                "Warning in " + file + " on line " + std::to_string(line) +
                ": " << msg << "\n";
            break;
        default:
            break;
        }
}

void Handler::write(std::string fileName, std::string label, const std::vector<unsigned char> &data, int address, size_t start, size_t length, bool printpc)
{
    addresses.addAddress({address, label, false});
    addresses.addFile(label, fileName, length, printpc);
    outputFile((baseDir / fileName).string(), data, length, start);
}

AddressList Handler::done()
{
    addresses.sort();
    return addresses;
}

int Handler::getNextAddress(const std::string & dir) const
{
    return addresses.getNextAddress(dir);
}

void Handler::setNextAddress(int nextAddress)
{
    addresses.setNextAddress(nextAddress);
}

}
