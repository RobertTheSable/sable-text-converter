#ifndef ROMPATCHER_H
#define ROMPATCHER_H
#include "mapping.h"
#include "util.h"
#include "table.h"
#include <vector>
#include <string>
#include "wrapper/filesystem.h"

namespace sable {

class RomPatcher
{
public:
    RomPatcher(const std::string& mode = "lorom");
    void loadRom(const std::string& file, const std::string& name, int header = 0);
    //~RomPatcher();
    bool expand(int maxAddress);
    bool applyPatchFile(const std::string& path, const std::string& format = "asm");
    unsigned char& at(int n);
    bool getMessages(std::back_insert_iterator<std::vector<std::string>> v);
    int getRealSize() const;


    std::string generateInclude(const fs::path& file, const fs::path& basePath, bool isBin) const;
    std::string generateDefine(const std::string& label) const;
    std::string generateNumber(int number, int width, int base = 16) const;
    std::string generateAssignment(const std::string& label, int value, int width, const std::string& baseLabel = "", int base = 16) const;

//    int getRomSize() const;

//    unsigned char &atROMAddr(int n);
//    std::string getName() const;

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
