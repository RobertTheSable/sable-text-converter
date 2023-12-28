#include <catch2/catch.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include "wrapper/filesystem.h"
#include "helpers.h"
#include "project/groupparser.h"

struct caseFile {
    fs::path name;
    std::string contents;
};

struct caseFileList
{
    fs::path folder;
    std::vector<fs::path> files;
    caseFileList(fs::path folder_): folder{folder_}
    {
        fs::create_directory(folder);
    }
    ~caseFileList()
    {
        fs::remove_all(folder);
    }

    void createFile(caseFile cf)
    {
        std::ofstream of((folder / cf.name).string());

        of << cf.contents;
        of.close();
        files.push_back(folder / cf.name);
    }
    template<typename ...Args>
    void create(Args&& ...cases)
    {
        (createFile(cases), ...);
    }
};

struct stubWriter {
    auto operator()() {}
};

struct stubIntParser {
    int index;
    struct {
        struct {
            sable::FontList fl;
            const sable::FontList& getFonts() const{
                return fl;
            }
        } m_Parser;
    } bp;
    stubIntParser(sable::FontList&& fl_, int startingIndex) {
        index = startingIndex;
        bp.m_Parser.fl = fl_;
    }

    template<class Handler>
    sable::FileParser::Result processFile(
        std::istream& input,
        const sable::util::Mapper& mapper,
        const std::string& currentDir,
        const std::string& fileKey,
        int nextAddress,
        int startingDirIndex,
        Handler h,
        stubWriter wr
    ) {

        int count = 0;
        input >> count;

        index = startingDirIndex;
        ++index;
        return {.dirIndex = index, .address = (count + nextAddress)};
    }
};

TEST_CASE("Test parsing a folder - using stubs.")
{
    using sable::GroupParser;
    caseFileList samplesFolder(fs::path("samples"));
    samplesFolder.create(
        caseFile{
            fs::path{"test1.txt"},
            "3\n"
        },
        caseFile{
            fs::path{"test2.txt"},
            "5\n"
        },
        caseFile{
            fs::path{"test3.txt"},
            "8\n"
        }
    );
    GroupParser<stubIntParser> gp(sable::FontList{}, 0);

    sable::files::Group gr("samples", samplesFolder.folder, [] (fs::path p) -> std::optional<fs::path> {
        return p;
    }, samplesFolder.files.begin(), samplesFolder.files.end());
    sable::util::Mapper m(sable::util::LOROM, false, false);
    int resultAddress = 0;

    REQUIRE_NOTHROW(resultAddress = gp.processGroup(gr, m, "samples", 800, 0, stubWriter{}));
    REQUIRE(resultAddress == 816);
    REQUIRE(gp.fp.index == 3);
    REQUIRE(gp.getFonts().size() == 0);
}

TEST_CASE("Test parsing a folder with a missing file - using stubs")
{
    using sable::GroupParser;
    caseFileList samplesFolder(fs::path("samples"));
    samplesFolder.create(
        caseFile{
            fs::path{"test1.txt"},
            "3\n"
        },
        caseFile{
            fs::path{"test2.txt"},
            "5\n"
        }
    );
    samplesFolder.files.push_back("test3.txt");

    GroupParser<stubIntParser> gp(sable::FontList{}, 0);

    sable::files::Group gr("samples", samplesFolder.folder, [] (fs::path p) -> std::optional<fs::path> {
        return p;
    }, samplesFolder.files.begin(), samplesFolder.files.end());
    sable::util::Mapper m(sable::util::LOROM, false, false);
    int resultAddress = 0;

    REQUIRE_THROWS_WITH(
        gp.processGroup(gr, m, "samples", 800, 0, stubWriter{}),
        "In samples: file test3.txt does not exist."
    );
}
