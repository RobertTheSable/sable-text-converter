#include <catch2/catch.hpp>

#include <sstream>

#include "project/folder.h"

#include "files.h"

using sable::files::Folder;

TEST_CASE("Folder with table")
{
    caseFileList cs("samples");

    std::ostringstream tableContents;

    tableContents
        << "address 808000\n"
           "width 3\n"
           "data e08000\n"
           "file 02.txt\n"
           "file 01.txt\n"
           "file 03.txt\n"
           "\n"
           "entry test_01\n"
           "entry test_02\n"
           "entry test_03\n"
    ;
    SECTION("no extra files")
    {
        cs.create(
            caseFile{fs::path("table.txt"), tableContents.str()},
            caseFile{fs::path("01.txt"), "test\n"},
            caseFile{fs::path("02.txt"), "test\n"},
            caseFile{fs::path("03.txt"), "test\n"}
        );
    }

    SECTION("one file and folder not in the table")
    {
        cs.create(
            caseFile{fs::path("table.txt"), tableContents.str()},
            caseFile{fs::path("01.txt"), "test\n"},
            caseFile{fs::path("02.txt"), "test\n"},
            caseFile{fs::path("03.txt"), "test\n"},
            caseFile{fs::path("04.txt"), "test\n"},
            "subfolder"
        );
    }

    sable::util::Mapper m(sable::util::LOROM, false, false);
    Folder f(cs.folder, m);
    REQUIRE(f.table);
    REQUIRE((f.group.end() - f.group.begin()) == 3);
    REQUIRE(f.table->getAddress() == 0x808000);
    REQUIRE(f.table->getDataAddress() == 0xe08000);
    std::vector expectedFiles{"samples/02.txt", "samples/01.txt", "samples/03.txt"};
    int index = 0;
    for (auto file: f.group) {
        REQUIRE(file.string() == expectedFiles[index]);
        ++index;
    }
}

TEST_CASE("Folder without table")
{
    caseFileList cs("samples");

    SECTION("no extra files")
    {
        cs.create(
            caseFile{fs::path("01.txt"), "test\n"},
            caseFile{fs::path("02.txt"), "test\n"},
            caseFile{fs::path("03.txt"), "test\n"}
        );
    }

    SECTION("one file and folder not in the table")
    {
        cs.create(
            caseFile{fs::path("01.txt"), "test\n"},
            caseFile{fs::path("02.txt"), "test\n"},
            caseFile{fs::path("03.txt"), "test\n"},
            "subfolder"
        );
    }

    sable::util::Mapper m(sable::util::LOROM, false, false);
    Folder f(cs.folder, m);
    REQUIRE(!f.table);
    std::vector expectedFiles{"samples/01.txt", "samples/02.txt", "samples/03.txt"};
    int index = 0;
    for (auto file: f.group) {
        REQUIRE(file.string() == expectedFiles[index]);
        ++index;
    }
}

TEST_CASE("Folder with table that can't be read")
{
    caseFileList cs("samples");

    cs.create(
        caseFile{fs::path("table.txt"), ""},
        caseFile{fs::path("01.txt"), "test\n"},
        caseFile{fs::path("02.txt"), "test\n"},
        caseFile{fs::path("03.txt"), "test\n"}
    );

    fs::permissions(cs.folder / "table.txt", fs::perms::owner_write|fs::perms::group_write, fs::perm_options::replace);

    sable::util::Mapper m(sable::util::LOROM, false, false);

    REQUIRE_THROWS_WITH(Folder(cs.folder, m), (fs::absolute(cs.folder) /"table.txt").string() + " could not be opened.");
}
