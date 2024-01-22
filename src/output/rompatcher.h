#ifndef ROMPATCHER_H
#define ROMPATCHER_H
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <functional>
#include <type_traits>

#include "wrapper/filesystem.h"
#include "data/addresslist.h"
#include "data/table.h"
#include "data/mapper.h"
#include "font/font.h"

namespace sable {

typedef std::vector<std::string>::const_iterator ConstStringIterator;

struct RomPatcher
{
    enum class AsarState {NotRun, Success, Error, InitFailed};
private:
    std::vector<unsigned char> m_data;
    int m_RomSize;
    int m_HeaderSize;
    sable::util::MapperType m_MapType;
    AsarState m_AState;
    void writeSingleFontData(const sable::Font& font, std::ostream& output) const;
public:
    static bool succeeded(AsarState state);
    static bool wasRun(AsarState state);
    RomPatcher(const util::MapperType& mapper = util::MapperType::LOROM);
    ~RomPatcher();
    bool loadRom(const std::string& file, const std::string& name, int header = 0);
    void clear();
    //~RomPatcher();
    bool expand(int size, const util::Mapper& mapper);
    AsarState applyPatchFile(const std::string& path, const std::string& format = "asm");
    unsigned char& at(int n);
    bool getMessages(std::back_insert_iterator<std::vector<std::string>> v);
    int getRealSize() const;

    void writeParsedData(const AddressList& addresses, const fs::path& includePath, std::ostream& mainText, std::ostream& textDefines);
    void writeInclude(const std::string include, std::ostream& mainFile, const fs::path& includePath = fs::path());
    void writeIncludes(ConstStringIterator start, ConstStringIterator end, std::ostream& mainFile, const fs::path& includePath = fs::path());
    template<class Fl>
    void writeFontData(Fl list, std::ostream& output) const
    {
        for (auto& fontIt: list) {
            auto font = fontIt.second;
            writeSingleFontData(font, output);
        }
    }

    std::string getMapperDirective(const util::MapperType& mapper);
//    int getRomSize() const;

//    unsigned char &atROMAddr(int n);
//    std::string getName() const;

};
}


#endif // ROMPATCHER_H
