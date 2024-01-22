#include <catch2/catch.hpp>

#include <vector>
#include <string>

#include "project/group.h"

#include "files.h"

using sable::files::Group;

TEST_CASE("Create group from folder")
{
    caseFileList cs("samples");
    cs.create(
        caseFile{fs::path("test1.txt"), "test\n"},
        caseFile{fs::path("test2.txt"), "test\n"},
        caseFile{fs::path("test3.txt"), "test\n"}
    );

    auto group = Group::fromList("samples", cs.folder, fs::directory_iterator(cs.folder), fs::directory_iterator());

    REQUIRE(group.getName() == "samples");
    REQUIRE(group.getPath() == cs.folder);

    std::size_t index = 0;
    for (auto file: group) {
        REQUIRE(file == cs.files[index]);
        ++index;
    }

    REQUIRE(index == 3);
}

TEST_CASE("Create group from folder - requires sorting")
{
    caseFileList cs("samples");
    cs.create(
        caseFile{fs::path("test2.txt"), "test\n"},
        caseFile{fs::path("test1.txt"), "test\n"},
        caseFile{fs::path("test3.txt"), "test\n"}
    );

    auto group = Group::fromList("samples", cs.folder, fs::directory_iterator(cs.folder), fs::directory_iterator());

    REQUIRE(group.getName() == "samples");
    REQUIRE(group.getPath() == cs.folder);

    std::vector expected{"test1.txt", "test2.txt", "test3.txt"};

    std::size_t index = 0;
    for (auto file: group) {
        REQUIRE(file.filename().string() == expected[index]);
        ++index;
    }

    REQUIRE(index == 3);
}


TEST_CASE("Create group from folder with folder inside it")
{
    caseFileList cs("samples");
    cs.create(
        caseFile{fs::path("test2.txt"), "test\n"},
        caseFile{fs::path("test1.txt"), "test\n"},
        caseFile{fs::path("test3.txt"), "test\n"},
        "test4"
    );

    const Group group = Group::fromList("samples", cs.folder, fs::directory_iterator(cs.folder), fs::directory_iterator());

    REQUIRE(group.getName() == "samples");
    REQUIRE(group.getPath() == cs.folder);

    std::vector expected{"test1.txt", "test2.txt", "test3.txt"};

    std::size_t index = 0;
    for (auto file: group) {
        REQUIRE(file.filename().string() == expected[index]);
        ++index;
    }

    REQUIRE(index == 3);
}


TEST_CASE("Create group from table")
{
    std::vector sample{"test1.txt", "test3.txt", "test2.txt"};

    auto group = Group::fromTable("table", fs::path("table"), sample.begin(), sample.end());

    REQUIRE(group.getName() == "table");
    REQUIRE(group.getPath() == fs::path("table"));

    std::vector expected{"table/test1.txt", "table/test3.txt", "table/test2.txt"};

    std::size_t index = 0;
    for (auto file: group) {
        REQUIRE(file.string() == expected[index]);
        ++index;
    }

    REQUIRE(index == 3);
}
