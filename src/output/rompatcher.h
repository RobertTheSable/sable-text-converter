#ifndef ROMPATCHER_H
#define ROMPATCHER_H
#include "util.h"
#include <vector>
#include <string>
#include <functional>
#include "wrapper/filesystem.h"
#include "data/addresslist.h"
#include "font/font.h"
#include "data/table.h"

namespace sable {

typedef std::vector<std::string>::const_iterator ConstStringIterator;

class RomPatcher
{
public:
    RomPatcher(const util::MapperType& mapper = util::MapperType::LOROM);
    ~RomPatcher();
    bool loadRom(const std::string& file, const std::string& name, int header = 0);
    void clear();
    //~RomPatcher();
    bool expand(int size, const util::Mapper& mapper);
    bool applyPatchFile(const std::string& path, const std::string& format = "asm");
    unsigned char& at(int n);
    bool getMessages(std::back_insert_iterator<std::vector<std::string>> v);
    int getRealSize() const;

    void writeParsedData(const AddressList& addresses, const fs::path& includePath, std::ostream& mainText, std::ostream& textDefines);
    void writeIncludes(ConstStringIterator start, ConstStringIterator end, std::ostream& mainFile, const fs::path& includePath = fs::path());
    template<class Fl>
    void writeFontData(Fl list, std::ostream& output)
    {
        for (auto& fontIt: list) {
            auto font = fontIt.second;
            if (!font.getFontWidthLocation().empty()) {
                output << "\n"
                          "ORG " + font.getFontWidthLocation();
                std::vector<int> widths;

                for (int pIdx = 0; pIdx < font.getNumberOfPages(); pIdx++) {
                                widths.reserve(font.getMaxEncodedValue(pIdx));
                    font.getFontWidths(pIdx, std::back_insert_iterator(widths));
                }

                int column = 0;
                int skipCount = 0;
                for (auto it = widths.begin(); it != widths.end(); ++it) {
                    int width = *it;
                    if (width == 0) {
                        skipCount++;
                        column = 0;
                    } else {
                        if (skipCount > 0) {
                            output << "\n"
                                      "skip " << std::dec << skipCount;
                            skipCount = 0;
                        }
                        if (column == 0) {
                            output << "\ndb ";
                        } else {
                            output << ", ";
                        }
                        output << "$" << std::hex << std::setw(2) << std::setfill('0') << width;
                        column++;
                        if (column ==16) {
                            column = 0;
                        }
                    }
                }
                output << '\n' ;
            }
        }
    }

    std::string getMapperDirective(const util::MapperType& mapper);
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
    sable::util::MapperType m_MapType;
    enum AsarState {NotRun, Success, Error};
    AsarState m_AState;
};
}


#endif // ROMPATCHER_H
