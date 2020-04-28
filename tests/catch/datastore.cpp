#include <catch2/catch.hpp>
#include "datastore.h"
#include "helpers.h"

using Catch::Matchers::Contains;

TEST_CASE("Text file parsing")
{
    sable::DataStore tester(sable_tests::getSampleNode(), "normal");
    std::istringstream input;
    std::ostringstream errors;
    std::vector<unsigned char> v;
    SECTION("Basic file")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        REQUIRE_NOTHROW(tester.addFile(input, fs::temp_directory_path() / "test.txt", errors));
        auto queueItem = tester.getOutputFile();
        REQUIRE(queueItem.first == "test1.bin");
        REQUIRE(tester.data_end() - tester.data_begin() == 9);
        v = std::vector<unsigned char>(tester.data_begin(), tester.data_end());
        REQUIRE(queueItem.second == v.size());
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 0);
        REQUIRE(tester.getOutputFile() == std::make_pair(std::string(""), 0));
        REQUIRE(errors.str().empty());
        REQUIRE(tester.end() - tester.begin() == 1);
        REQUIRE(tester.begin()->address == 0x808000);
        REQUIRE_FALSE(tester.begin()->isTable);
        REQUIRE(tester.begin()->label == "test1");
    }
    SECTION("File that crosses banks")
    {
        input.str("@address 80FFFF\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
        v = std::vector<unsigned char>(tester.data_begin(), tester.data_end());
        REQUIRE(v.size() == 9);
        auto queueItem = tester.getOutputFile();
        auto queueItem2 = tester.getOutputFile();
        REQUIRE(queueItem.second + queueItem2.second == v.size());
        REQUIRE(queueItem == std::make_pair(std::string("test1.bin"), 1));
        REQUIRE(queueItem2 == std::make_pair(std::string("test1bank.bin"), 8));
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 0);
        REQUIRE(tester.end() - tester.begin() == 2);
        tester.sort();
        REQUIRE(tester.begin()->address == 0x80FFFF);
        REQUIRE((++tester.begin())->address == 0x818000);
    }
    SECTION("File with multiple pieces of text")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG[End]\n"
                  "@label test2\n"
                  "ABCDEFG\n");
        tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
        REQUIRE(tester.end() - tester.begin() == 2);
        REQUIRE((++tester.begin())->address == 0x808009);
        REQUIRE((++tester.begin())->label == "test2");
        REQUIRE(tester.data_end() - tester.data_begin() == 18);
        std::pair<std::string, int> result;
        std::pair<std::string, int> files[2] = {tester.getOutputFile(), tester.getOutputFile()};

        REQUIRE(*tester.data_begin() == 1);
        REQUIRE(*(tester.data_begin()+files[0].second) == 1);
        REQUIRE(*(tester.data_end() -1) == 0);
        REQUIRE(*(tester.data_end() - (files[0].second+1)) == 0);
        REQUIRE(tester.data_end() - tester.data_begin() == files[0].second + files[1].second);
        REQUIRE_NOTHROW(tester.getFile("test1"));
        REQUIRE_NOTHROW(tester.getFile("test2"));
    }
    SECTION("File with no label")
    {
        input.str("@address 808000\n"
                  "ABCDEFG[End]\n"
                  "ABCDEFG\n");
        tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
        REQUIRE(tester.getOutputFile().first == fs::temp_directory_path().filename().string() + "_0.bin");
        REQUIRE(tester.getOutputFile().first == fs::temp_directory_path().filename().string() + "_1.bin");
    }
    SECTION("File ending with a comment")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n"
                  "XYZ");
        tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
        REQUIRE(tester.data_end() - tester.data_begin() == 14);
        REQUIRE(tester.getOutputFile().second == 14);
    }

    SECTION("Parse two files from the same path")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
        tester.getOutputFile();
        input.clear();
        input.str("@label test2\n"
                  "TUVWXYZ\n");
        tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
        REQUIRE(tester.getNextAddress() == 0x808000 + 18);
        REQUIRE((++tester.begin())->address == 0x808009);
        REQUIRE((++tester.begin())->label == "test2");

    }
}

TEST_CASE("Parse with an alternate default mode")
{
    sable::DataStore tester(sable_tests::getSampleNode(), "menu");
    std::istringstream input;
    std::ostringstream errors;
    std::vector<unsigned char> v;
    input.str("@address 808000\n"
              "@label test1\n"
              "ABCDEFG\n");
    tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
    REQUIRE(*tester.data_begin() == 0);
    REQUIRE(*(tester.data_end() -1) == 0xFF);
    REQUIRE(tester.data_end() - tester.data_begin() == 16);

}

TEST_CASE("Table adding")
{
    sable::DataStore tester(sable_tests::getSampleNode(), "normal");
    std::istringstream input;
    std::ostringstream errors;
    std::vector<unsigned char> v;
    input.str("@address 808000\n"
              "@label test1\n"
              "ABCDEFG\n");
    tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
}

TEST_CASE("Data store error handling")
{
    sable::DataStore tester(sable_tests::getSampleNode(), "normal");
    std::istringstream input;
    std::vector<unsigned char> v = {};
    std::ostringstream errors;
    SECTION("Long line detection")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG\n");
        tester.addFile(input, fs::temp_directory_path() / "test.txt", errors);
        REQUIRE_THAT(errors.str(), Contains(" on line 3: Line is longer than the specified max width of 160 pixels."));
        REQUIRE(tester.data_end() - tester.data_begin() == 95);

    }
    SECTION("Invalid file handling")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "%");
        REQUIRE_THROWS(tester.addFile(input, fs::temp_directory_path() / "test.txt", errors));
    }
    SECTION("Invalid address handling")
    {
        tester.setNextAddress(0x7e0000);
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        REQUIRE_THROWS_WITH(
                    tester.addFile(input, fs::temp_directory_path() / "test.txt", errors),
                    "Attempted to begin parsing with invalid ROM address $7e0000");
    }
    SECTION("Missing file label.")
    {
        REQUIRE_THROWS_WITH(tester.getFile("nothing"), "Label \"nothing\" not found.");
    }
}
