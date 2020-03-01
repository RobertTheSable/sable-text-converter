#ifndef ROMPATCHER_H
#define ROMPATCHER_H
#include "mapping.h"
#include "util.h"
#include <vector>
#include <string>

namespace sable {

class RomPatcher
{
public:
    RomPatcher(const std::string& file, const std::string& name, const std::string& mode, int header = 0);
    //~RomPatcher();
    bool expand(int maxAddress);
    bool applyPatchFile(const std::string& path, const std::string& format = "asm");
    int getRomSize() const;
    int getRealSize() const;
    unsigned char& at(int n);
    unsigned char &atROMAddr(int n);
    std::string getName() const;
    bool getMessages(std::back_insert_iterator<std::vector<std::string>> v);

private:
    std::vector<unsigned char> m_data;
    std::string m_Name;
    int m_RomSize;
    int m_HeaderSize;
    Mapper m_MapType;
    enum AsarState {NotRun, Success, Error};
    AsarState m_AState;
};
}


#endif // ROMPATCHER_H
