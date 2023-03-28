#include <catch2/catch.hpp>
#include "datastore.h"
#include "helpers.h"

using Catch::Matchers::Contains;

sable::TextParser getParser(const std::string& defaultMode)
{;
    return sable::TextParser(
        sable_tests::getSampleNode().as<sable::FontList>(),
        defaultMode,
        "en_US.utf-8"
    );
}

TEST_CASE("Loading script data from files")
{
    sable::DataStore dataTester(getParser("normal"));
    std::istringstream input;
    std::ostringstream errors;
    std::vector<unsigned char> v;
    sable::util::Mapper m(sable::util::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    SECTION("Basic file")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        REQUIRE_NOTHROW(dataTester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m));
        auto queueItem = dataTester.getOutputFile();
        REQUIRE(queueItem.first == "test1.bin");
        REQUIRE(dataTester.data_end() - dataTester.data_begin() == 9);
        v = std::vector<unsigned char>(dataTester.data_begin(), dataTester.data_end());
        REQUIRE(queueItem.second == v.size());
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 0);
        REQUIRE(dataTester.getOutputFile() == std::make_pair(std::string(""), 0));
        REQUIRE(errors.str().empty());
        REQUIRE(dataTester.end() - dataTester.begin() == 1);
        REQUIRE(dataTester.begin()->address == 0x808000);
        REQUIRE_FALSE(dataTester.begin()->isTable);
        REQUIRE(dataTester.begin()->label == "test1");
    }
    SECTION("File that crosses banks")
    {
        input.str("@address 80FFFF\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        dataTester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m);
        v = std::vector<unsigned char>(dataTester.data_begin(), dataTester.data_end());
        REQUIRE(v.size() == 9);
        auto queueItem = dataTester.getOutputFile();
        auto queueItem2 = dataTester.getOutputFile();
        REQUIRE(queueItem.second + queueItem2.second == v.size());
        REQUIRE(queueItem == std::make_pair(std::string("test1.bin"), 1));
        REQUIRE(queueItem2 == std::make_pair(std::string("test1bank.bin"), 8));
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 0);
        REQUIRE(dataTester.end() - dataTester.begin() == 2);
        dataTester.sort();
        REQUIRE(dataTester.begin()->address == 0x80FFFF);
        REQUIRE((++dataTester.begin())->address == 0x818000);
    }
    SECTION("File with multiple pieces of text")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG[End]\n"
                  "@label test2\n"
                  "ABCDEFG\n");
        dataTester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m);
        REQUIRE(dataTester.end() - dataTester.begin() == 2);
        REQUIRE((++dataTester.begin())->address == 0x808009);
        REQUIRE((++dataTester.begin())->label == "test2");
        REQUIRE(dataTester.data_end() - dataTester.data_begin() == 18);
        std::pair<std::string, int> result;
        std::pair<std::string, int> files[2] = {dataTester.getOutputFile(), dataTester.getOutputFile()};

        REQUIRE(*dataTester.data_begin() == 1);
        REQUIRE(*(dataTester.data_begin()+files[0].second) == 1);
        REQUIRE(*(dataTester.data_end() -1) == 0);
        REQUIRE(*(dataTester.data_end() - (files[0].second+1)) == 0);
        REQUIRE(dataTester.data_end() - dataTester.data_begin() == files[0].second + files[1].second);
        REQUIRE_NOTHROW(dataTester.getFile("test1"));
        REQUIRE_NOTHROW(dataTester.getFile("test2"));
    }
    SECTION("File with no label")
    {
        input.str("@address 808000\n"
                  "ABCDEFG[End]\n"
                  "ABCDEFG\n");
        dataTester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m);
        REQUIRE(dataTester.getOutputFile().first == fs::temp_directory_path().filename().string() + "_0.bin");
        REQUIRE(dataTester.getOutputFile().first == fs::temp_directory_path().filename().string() + "_1.bin");
    }
    SECTION("File ending with a comment")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n"
                  "XYZ");
        dataTester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m);
        REQUIRE(dataTester.data_end() - dataTester.data_begin() == 14);
        REQUIRE(dataTester.getOutputFile().second == 14);
    }

    SECTION("Parse two files from the same path")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        dataTester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m);
        dataTester.getOutputFile();
        input.clear();
        input.str("@label test2\n"
                  "TUVWXYZ\n");
        dataTester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m);
        REQUIRE(dataTester.getNextAddress() == 0x808000 + 18);
        REQUIRE((++dataTester.begin())->address == 0x808009);
        REQUIRE((++dataTester.begin())->label == "test2");

    }
}

TEST_CASE("Parse with an alternate default mode")
{
    sable::DataStore tester(getParser("menu"));
    sable::util::Mapper m(sable::util::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    std::istringstream input;
    std::ostringstream errors;
    std::vector<unsigned char> v;
    input.str("@address 808000\n"
              "@label test1\n"
              "ABCDEFG\n");
    tester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m);
    REQUIRE(*tester.data_begin() == 0);
    REQUIRE(*(tester.data_end() -1) == 0xFF);
    REQUIRE(tester.data_end() - tester.data_begin() == 16);

}

TEST_CASE("Add files that are part of a table")
{
    sable::util::Mapper m(sable::util::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    sable::DataStore tester(getParser("menu"));
    std::istringstream input;
    std::ostringstream error;
    SECTION("Table with data immediatley after")
    {
        input.str("address $808000 \n"
                  "width 2 \n"
                  "savewidth \n"
                  "entry test_1 \n"
                  "entry test_2 \n"
                  "entry const $8080 2 \n\n"
                  "file file1.txt \n"
                  "file file2.txt \n"
                  "file file3.txt \n");
        tester.addTable(input, fs::temp_directory_path() / "table.txt");
        input.clear();
        input.str("@address auto\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        tester.addFile(input, fs::temp_directory_path() / "file1.txt", error, m);
        REQUIRE(tester.end() - tester.begin() == 2);
        REQUIRE(tester.begin()->address == 0x808000);
        REQUIRE(tester.begin()->isTable);
        REQUIRE((++tester.begin())->address == 0x808000 + 12);
        REQUIRE_FALSE((++tester.begin())->isTable);
        REQUIRE(error.str().empty());
    }
    SECTION("Table with data address")
    {
        input.str("address $808000 \n"
                  "data $908000 \n"
                  "width 2 \n"
                  "savewidth \n"
                  "entry test_1 \n"
                  "entry test_2 \n"
                  "entry const $8080 2 \n\n"
                  "file file1.txt \n"
                  "file file2.txt \n"
                  "file file3.txt \n");
        tester.addTable(input, fs::temp_directory_path() / "table.txt");
        input.clear();
        input.str("@address auto\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        tester.addFile(input, fs::temp_directory_path() / "file1.txt", error, m);
        REQUIRE(tester.end() - tester.begin() == 2);
        REQUIRE(tester.begin()->address == 0x808000);
        REQUIRE(tester.begin()->isTable);
        REQUIRE((++tester.begin())->address == 0x908000);
        REQUIRE_FALSE((++tester.begin())->isTable);
        REQUIRE(error.str().empty());
    }
}

TEST_CASE("Table adding")
{
    sable::DataStore tester(getParser("normal"));
    std::istringstream input;
    std::ostringstream errors;
    std::vector<unsigned char> v;

    input.str("address $808000 \n"
              "data $908000 \n"
              "width 2 \n"
              "savewidth \n"
              "entry test_1 \n"
              "entry test_2 \n"
              "entry const $8080 2 \n\n"
              "file file1.txt \n"
              "file file2.txt \n"
              "file file3.txt \n");
    tester.addTable(input, fs::temp_directory_path() / "table.txt");
    REQUIRE(tester.end() - tester.begin() == 1);
    REQUIRE(tester.getTable(fs::temp_directory_path().filename().string()).getSize() == 12);
}

TEST_CASE("Sorting addresses")
{
    sable::DataStore tester(getParser("normal"));
    std::istringstream input;
    std::ostringstream errors;
    std::vector<unsigned char> v;
    REQUIRE_FALSE(tester.getIsSorted());
    input.str("address $908000 \n"
              "data $808000 \n"
              "width 2 \n"
              "savewidth \n"
              "entry test_1 \n"
              "entry test_2 \n"
              "entry const $8080 2 \n\n"
              "file file1.txt \n"
              "file file2.txt \n"
              "file file3.txt \n");
    tester.addTable(input, fs::temp_directory_path() / "table.txt");
    input.clear();
    input.str("@address auto\n"
              "@label test1\n"
              "ABCDEFG\n");
    REQUIRE_FALSE(tester.getIsSorted());
    tester.sort();
    REQUIRE(tester.getIsSorted());
    int last = 0;
    for (auto& it : tester) {
        REQUIRE(last <= it.address);
        last = it.address;
    }

}

TEST_CASE("Data store error handling")
{
    sable::DataStore tester(getParser("normal"));
    sable::util::Mapper m(sable::util::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    std::istringstream input;
    std::vector<unsigned char> v = {};
    std::ostringstream errors;
    SECTION("Long line detection")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG\n");
        tester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m);
        REQUIRE_THAT(errors.str(), Contains(" on line 3: Line is longer than the specified max width of 160 pixels."));
        REQUIRE(tester.data_end() - tester.data_begin() == 95);

    }
    SECTION("Invalid file handling")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "%");
        REQUIRE_THROWS(tester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m));
    }
    SECTION("Invalid address handling")
    {
        tester.setNextAddress(0x7e0000);
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        REQUIRE_THROWS_WITH(
                    tester.addFile(input, fs::temp_directory_path() / "test.txt", errors, m),
                    "Attempted to begin parsing with invalid ROM address $7e0000");
    }
    SECTION("Missing file label.")
    {
        REQUIRE_THROWS_WITH(tester.getFile("nothing"), "Label \"nothing\" not found.");
    }
}
