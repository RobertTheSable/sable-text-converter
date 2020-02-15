#include <catch2/catch.hpp>
#include <sstream>
#include "table.h"

TEST_CASE("Test that Table size is computed correctly.", "[table]")
{
    sable::Table t;
    t.addEntry(0x8000, 3);
    t.addEntry("Test 1");
    t.addEntry(0x8000, 20);
    t.addEntry(0x8000, 90);
    SECTION("With no widths stored")
    {
        t.setAddressSize(2);
        REQUIRE(t.getSize() == 8);
    }
    SECTION("With widths stored")
    {
        t.setAddressSize(2);
        t.setStoreWidths(true);
        REQUIRE(t.getSize() == 16);
    }
    SECTION("With long addresses")
    {
        t.setAddressSize(3);
        REQUIRE(t.getSize() == 12);
    }
    SECTION("With long addresses and stored widths")
    {
        t.setAddressSize(3);
        t.setStoreWidths(true);
        REQUIRE(t.getSize() == 24);
    }
    SECTION("Test that the correct number of entries are stored")
    {
        int test = 0;
        for(auto it = t.begin(); it != t.end(); ++it){
            test++;
        }
        REQUIRE(test == 4);
    }
}

TEST_CASE("Test that loading a table file produces correct results.", "[table]")
{
    sable::Table t;
    std::stringstream sampleFile;
    sampleFile << "address 808000\n";
    SECTION("Width 2, 2 files, 3 entries, no data address")
    {
        sampleFile << "width 2\n"
                      "file a.txt\n"
                      "file b.txt\n\n"
                      "entry sample01\n"
                      "entry sample02\n"
                      "entry sample03\n";
        auto v = t.getDataFromFile(sampleFile);
        REQUIRE(t.getAddress() == 0x808000);
        REQUIRE(t.getDataAddress() == 0x808006);
        REQUIRE(v.size() == 2);
    }
    SECTION("Width 3, 2 files, 3 entries, no data address")
    {
        sampleFile << "width 3\n"
                      "file a.txt\n"
                      "file b.txt\n\n"
                      "entry sample01\n"
                      "entry sample02\n"
                      "entry sample03\n";
        auto v = t.getDataFromFile(sampleFile);
        REQUIRE(t.getAddress() == 0x808000);
        REQUIRE(t.getDataAddress() == 0x808009);
        REQUIRE(v.size() == 2);
    }
    SECTION("Width 3, 2 files, 3 entries, specified data address")
    {
        sampleFile << "width 3\n"
                      "data 818000\n"
                      "file a.txt\n"
                      "file b.txt\n\n"
                      "entry sample01\n"
                      "entry sample02\n"
                      "entry sample03\n";
        auto v = t.getDataFromFile(sampleFile);
        REQUIRE(t.getAddress() == 0x808000);
        REQUIRE(t.getDataAddress() == 0x818000);
        REQUIRE(v.size() == 2);
    }
}

TEST_CASE("Test that invalid input produces exceptions")
{
    sable::Table t;
    std::stringstream sampleFile;
    SECTION("Invalid address throws an error.")
    {
        sampleFile << "address 818r00\n"
                      "width 3\n"
                      "file b.txt\n\n"
                      "entry sample01\n";
        REQUIRE_THROWS_AS(t.getDataFromFile(sampleFile), std::runtime_error);
    }
    SECTION("Invalid data address throws an error.")
    {
        sampleFile << "address 81800\n"
                      "width 3\n"
                      "data 818r00\n"
                      "file b.txt\n\n"
                      "entry sample01\n";
        REQUIRE_THROWS_AS(t.getDataFromFile(sampleFile), std::runtime_error);
    }
}
