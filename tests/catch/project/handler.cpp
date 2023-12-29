#include <catch2/catch.hpp>
#include <sstream>
#include <fstream>

#include "project/handler.h"

#include "helpers.h"
#include "files.h"

using sable::Handler;

TEST_CASE("Write files")
{
    caseFileList cs("samples");
    std::ostringstream sink;
    Handler subject(fs::path("samples"), sink, sable_tests::getSampleFonts(), "normal", "en_US.utf-8");
    std::vector<unsigned char> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    SECTION("Single file")
    {
        REQUIRE_NOTHROW(subject.write("test1.bin", "test1", data, 0x808000, 0, 10, false));

        auto addresses = subject.done();
        REQUIRE(addresses.end() - addresses.begin() == 1);
        auto f = addresses.getFile("test1");
        REQUIRE(f.files == "test1.bin");
        REQUIRE(f.size == 10);
        REQUIRE(f.printpc == false);

        REQUIRE(fs::exists(cs.folder / "test1.bin"));
        std::ifstream inFile((cs.folder / "test1.bin").string(), std::ios_base::in|std::ios_base::ate);
        REQUIRE(inFile);
        auto p = inFile.tellg();
        inFile.seekg(0, std::ios_base::beg);
        REQUIRE((p - inFile.tellg()) == 10);
        inFile.close();
    }
    SECTION("Two files")
    {
        REQUIRE_NOTHROW(subject.write("test2.bin", "test2", data, 0x808010, 4, 6, false));
        REQUIRE_NOTHROW(subject.write("test1.bin", "test1", data, 0x808000, 0, 4, true));

        auto addresses = subject.done();
        REQUIRE(addresses.end() - addresses.begin() == 2);

        // ensuring that the addresses were sorted
        auto itr = addresses.begin();
        REQUIRE(itr->address == 0x808000);
        REQUIRE(itr->label == "test1");
        ++itr;
        REQUIRE(itr != addresses.end());
        REQUIRE(itr->address == 0x808010);
        REQUIRE(itr->label == "test2");


        auto f = addresses.getFile("test1");
        REQUIRE(f.files == "test1.bin");
        REQUIRE(f.size == 4);
        REQUIRE(f.printpc == true);

        f = addresses.getFile("test2");
        REQUIRE(f.files == "test2.bin");
        REQUIRE(f.size == 6);
        REQUIRE(f.printpc == false);


        std::ifstream inFile((cs.folder / "test1.bin").string(), std::ios_base::in|std::ios_base::ate);
        REQUIRE(inFile);
        auto p = inFile.tellg();
        inFile.seekg(0, std::ios_base::beg);
        REQUIRE((p - inFile.tellg()) == 4);
        inFile.close();

        inFile.open((cs.folder / "test2.bin").string(), std::ios_base::in|std::ios_base::ate);
        REQUIRE(inFile);
        p = inFile.tellg();
        inFile.seekg(0, std::ios_base::beg);
        REQUIRE((p - inFile.tellg()) == 6);
        inFile.close();
    }
    REQUIRE(sink.str() == "");
}

TEST_CASE("Write files - failed")
{
    caseFileList cs("samples");
    cs.add("samples");
    std::ostringstream sink;
    Handler subject(fs::path("samples"), sink, sable_tests::getSampleFonts(), "normal", "en_US.utf-8");
    std::vector<unsigned char> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    REQUIRE_THROWS(subject.write("samples", "test1", data, 0x808000, 0, 10, false));
}

TEST_CASE("Address setting")
{
    std::ostringstream sink;
    Handler subject(fs::path("samples"), sink, sable_tests::getSampleFonts(), "normal", "en_US.utf-8");
    sable::Table tl{};
    tl.setDataAddress(0x908000);

    subject.addresses.addTable("table test", std::move(tl));
    REQUIRE_NOTHROW(subject.setNextAddress(0x808000));
    REQUIRE(subject.getNextAddress("") == 0x808000);
    REQUIRE(subject.getNextAddress("table test") == 0x908000);
    REQUIRE(subject.getNextAddress("table which does not exist") == 0x808000);
    REQUIRE(sink.str() == "");
}

TEST_CASE("Error handling.")
{
    using sable::error::Levels;
    std::ostringstream sink;
    Handler subject(fs::path("samples"), sink, sable_tests::getSampleFonts(), "normal", "en_US.utf-8");
    SECTION("Level: Error")
    {
        REQUIRE_THROWS_WITH(subject.report("test.txt", Levels::Error, "some error", 10), "Error in text file test.txt, line 10: some error");
        REQUIRE_THROWS_AS(subject.report("test.txt", Levels::Error, "some error", 10), sable::ParseError);
        REQUIRE(sink.str() == "");
    }
    SECTION("Level: Warning")
    {
        REQUIRE_NOTHROW(subject.report("test.txt", Levels::Warning, "some warning", 10));
        REQUIRE(sink.str() == "Warning in test.txt on line 10: some warning\n");
    }
    SECTION("Level: Info")
    {
        REQUIRE_NOTHROW(subject.report("test.txt", Levels::Info, "some warning", 10));
        REQUIRE(sink.str() == "");
    }
}
