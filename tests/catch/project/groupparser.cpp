#include <catch2/catch.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include "wrapper/filesystem.h"
#include "helpers.h"
#include "files.h"
#include "project/groupparser.h"

struct stubIntParser {
    int index;
    int address;
    std::map<std::string, sable::Font> fl;

    const std::map<std::string, sable::Font>& getFonts() const{
        return fl;
    }

    stubIntParser(std::map<std::string, sable::Font>&& fl_, int startingIndex, int startingAddress) {
        index = startingIndex;
        fl = fl_;
        address = startingAddress;
    }
    int getNextAddress(const std::string & dir) const

    {
        return address;
    }
    void setNextAddress(int nextAddress)
    {
        address = nextAddress;
    }

    sable::parse::FileResult processFile(
        std::istream& input,
        const sable::util::Mapper& mapper,
        const std::string& currentDir,
        const std::string& fileKey,
        int nextAddress,
        int startingDirIndex
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
    stubIntParser stub(std::map<std::string, sable::Font>{}, 0, 800);
    GroupParser<stubIntParser> gp(stub);

    std::vector<std::string> files{"test1.txt", "test2.txt", "test3.txt"};

    auto gr = sable::files::Group::fromTable("samples", samplesFolder.folder, files.begin(), files.end());
    sable::util::Mapper m(sable::util::LOROM, false, false);

    REQUIRE_NOTHROW(gp.processGroup(gr, m, "samples", 0));
    REQUIRE(stub.address == 816);
    REQUIRE(stub.index == 3);
    REQUIRE(stub.getFonts().size() == 0);
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
    std::vector<std::string> files{"test1.txt", "test2.txt", "test3.txt"};


    stubIntParser stub(std::map<std::string, sable::Font>{}, 0, 800);
    GroupParser<stubIntParser> gp(stub);

    auto gr = sable::files::Group::fromTable("samples", samplesFolder.folder, files.begin(), files.end());
    sable::util::Mapper m(sable::util::LOROM, false, false);
    int resultAddress = 0;

    REQUIRE_THROWS_WITH(
        gp.processGroup(gr, m, "samples", 0),
        "In samples: file samples/test3.txt does not exist."
    );
}
