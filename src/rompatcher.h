#ifndef ROMPATCHER_H
#define ROMPATCHER_H
#include "mapping.h"
#include "util.h"
#include "table.h"
#include <vector>
#include <string>
#include <functional>
#include "wrapper/filesystem.h"
#include "datastore.h"

namespace sable {

typedef std::vector<std::string>::const_iterator ConstStringIterator;

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

    void writeParsedData(const DataStore& data, const fs::path& includePath, std::ostream& mainText, std::ostream& textDefines);
    void writeIncludes(ConstStringIterator start, ConstStringIterator end, std::ostream& mainFile, const fs::path& includePath = fs::path());
    void writeFontData(const DataStore& data, std::ofstream& output);

    std::string generateInclude(const fs::path& file, const fs::path& basePath, bool isBin) const;
    std::string generateDefine(const std::string& label) const;
    std::string generateAssignment(const std::string& label, int value, int width, int base = 16, const std::string& baseLabel = "") const;

//    int getRomSize() const;

//    unsigned char &atROMAddr(int n);
//    std::string getName() const;

private:
    std::string generateNumber(int number, int width, int base = 16) const;
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
