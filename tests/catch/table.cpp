#include <catch2/catch.hpp>
#include <sstream>
#include "table.h"

using Catch::Matchers::Contains;

TEST_CASE("Table properties.", "[table]")
{
    sable::Table t;
    SECTION("Default values.")
    {
        REQUIRE(t.getAddressSize() == 3);
        REQUIRE(t.getSize() == 0);
        REQUIRE(t.getAddress() == 0);
        REQUIRE(t.getEntryCount() == 0);
        REQUIRE(t.getDataAddress() == 0);
        REQUIRE(!t.getStoreWidths());
    }
    SECTION("Adding entries")
    {
        t.addEntry(0x8000, 3);
        REQUIRE(t.getSize() == 3);
        t.addEntry("Test");
        REQUIRE(t.getSize() == 6);
        REQUIRE(t.getEntryCount() == 2);
    }
    SECTION("Change properties")
    {
        t.setAddressSize(2);
        t.addEntry(0x8000, 3);
        REQUIRE(t.getSize() == 2);
        t.addEntry("Test");
        REQUIRE(t.getSize() == 4);
        t.setStoreWidths(true);
        REQUIRE(t.getStoreWidths());
        REQUIRE(t.getSize() == 8);

        auto it = t.begin();
        REQUIRE(it->size == 3);
        REQUIRE(it->address == 0x8000);
        REQUIRE(it->label.empty());
        REQUIRE(it != t.end());
        auto entry = *(++it);
        REQUIRE(entry.label == "Test");
        REQUIRE(entry.size == -1);
        REQUIRE(entry.address == -1);
        REQUIRE(!(++it != t.end()));
    }

}

TEST_CASE("Second constructor", "[table]")
{
    sable::Table t(2, true);
    SECTION("No further changes.")
    {
        REQUIRE(t.getStoreWidths());
        REQUIRE(t.getAddressSize() == 2);
        REQUIRE(t.getSize() == 0);
        REQUIRE(t.getAddress() == 0);
        t.setAddress(0xA08000);
        REQUIRE(t.getAddress() == 0xA08000);
        REQUIRE(t.getDataAddress() == 0);
        t.setDataAddress(0xB08000);
        REQUIRE(t.getDataAddress() == 0xB08000);
        t.addEntry("Test 1");
        REQUIRE(t.getSize() == 4);
        t.addEntry("Test 2");
        REQUIRE(t.getSize() == 8);
        REQUIRE(t.getEntryCount() == 2);
    }
    SECTION("Change address size")
    {
        t.setAddressSize(3);
        t.addEntry("Test 1");
        REQUIRE(t.getSize() == 6);
        t.addEntry("Test 2");
        REQUIRE(t.getSize() == 12);
    }
}

TEST_CASE("Loading data from stream", "[table]")
{
    sable::Table t;
    std::istringstream input;
    SECTION("File loading")
    {
        input.str("file a.txt");
        auto v = t.getDataFromFile(input);
        REQUIRE(v.front() == "a.txt");
        REQUIRE(v.size() == 1);
        REQUIRE(!(t.begin() != t.end()));
    }
    SECTION("Add entry.")
    {
        input.str("entry test_1");
        auto v = t.getDataFromFile(input);
        REQUIRE(t.getSize() == 3);
        REQUIRE(v.empty());
        REQUIRE(t.getEntryCount() == 1);
        auto it = t.begin();
        REQUIRE(it->size == -1);
        REQUIRE(it->address == -1);
        REQUIRE(it->label == "test_1");
    }
    SECTION("Add constant entry.")
    {
        input.str("entry const $D745, 0");
        REQUIRE(t.getDataFromFile(input).empty());
        auto it = t.begin();
        REQUIRE(it->size == -1);
        REQUIRE(it->address == 0xD745);
        REQUIRE(it->label.empty());
        REQUIRE(input.eof());
    }
    SECTION("Add constant entry with widths.")
    {
        t.setStoreWidths(true);
        input.str("entry const $D745, $0000");
        REQUIRE(t.getDataFromFile(input).empty());
        auto it = t.begin();
        REQUIRE(it->size == 0);
        REQUIRE(it->address == 0xD745);
        REQUIRE(it->label.empty());
        REQUIRE(input.eof());
    }
    SECTION("Update Settings")
    {
        input.str("address $808000");
        t.getDataFromFile(input);
        REQUIRE(t.getAddress() == 0x808000);
        REQUIRE(t.getDataAddress() == 0x808000);
        input.clear();
        input.str("data $908000");
        t.getDataFromFile(input);
        REQUIRE(t.getAddress() == 0x808000);
        REQUIRE(t.getDataAddress() == 0x908000);
        input.clear();
        input.str("width 2");
        t.getDataFromFile(input);
        REQUIRE(t.getAddressSize() == 2);
        input.clear();
        input.str("width 3");
        t.getDataFromFile(input);
        REQUIRE(t.getAddressSize() == 3);
        input.clear();
        input.str("savewidth ");
        t.getDataFromFile(input);
        REQUIRE(t.getStoreWidths());
    }
}

TEST_CASE("Loading data from multiline stream", "[table]")
{
    sable::Table t;
    std::istringstream input;
    SECTION("Stream with default settings")
    {
        input.str("address $808000\r\n"
                  "entry test_1\r\n"
                  "entry test_2\r\n"
                  "entry const $808000\r\n\r\n"
                  "file file1.txt\r\n"
                  "file file2.txt\r\n");
        auto v = t.getDataFromFile(input);
        REQUIRE(v.size() == 2);
        REQUIRE(t.getEntryCount() == 3);
        REQUIRE(t.getAddressSize() == 3);
        REQUIRE(t.getAddress() == 0x808000);
        REQUIRE(t.getDataAddress() == 0x808009);
        REQUIRE(!t.getStoreWidths());
        auto it = t.begin();
        REQUIRE(it->label == "test_1");
        REQUIRE((++it)->label == "test_2");
        REQUIRE((++it)->label.empty());
        REQUIRE(!(++it != t.end()));
    }
    SECTION("Stream with updated settings")
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
        auto v = t.getDataFromFile(input);
        REQUIRE(t.getAddress() == 0x808000);
        REQUIRE(t.getDataAddress() == 0x908000);
        REQUIRE(t.getStoreWidths());
        REQUIRE(v.size() == 3);
        REQUIRE(t.getEntryCount() == 3);
        REQUIRE(t.getAddressSize() == 2);
        auto it = t.begin();
        REQUIRE(it->label == "test_1");
        REQUIRE(it->size == -1);
        REQUIRE((++it)->label == "test_2");
        REQUIRE(it->address == -1);
        REQUIRE((++it)->label.empty());
        REQUIRE(it->address == 0x8080);
        REQUIRE(it->size == 2);
        REQUIRE(!(++it != t.end()));
    }
}

TEST_CASE("Error checking", "[table]")
{
    sable::Table t;
    std::istringstream wrongAddress;
    SECTION("Address errors")
    {
        wrongAddress.str("address \n");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" missing value for table address."));
        wrongAddress.clear();
        wrongAddress.str("address bad");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" is not a valid SNES address."));
        wrongAddress.clear();
        wrongAddress.str("address $000000");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" is not a valid SNES address."));
    }
    SECTION("Data address errors")
    {
        wrongAddress.str("data \n");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" missing address for table data address setting."));
        wrongAddress.clear();
        wrongAddress.str("data bad");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" is not a valid SNES address."));
        wrongAddress.clear();
        wrongAddress.str("data $000000");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" is not a valid SNES address."));
    }
    SECTION("Width errors")
    {
        wrongAddress.str("width \n");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains("missing value for table width setting."));
        wrongAddress.clear();
        wrongAddress.str("width bad");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains("width value should be 2 or 3."));
        wrongAddress.clear();
        wrongAddress.str("width 4");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains("width value should be 2 or 3."));
    }
    SECTION("Unsupported settings.")
    {
        wrongAddress.str("notSupported");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains("unrecognized setting \"notSupported\""));
    }
    SECTION("Data errors")
    {
        wrongAddress.str("entry \n"
                         "entry test");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" missing data for table entry."));
        wrongAddress.str("file \n");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" missing filename."));

    }
    SECTION("Constant entry errors")
    {
        t.setAddressSize(2);
        wrongAddress.str("entry const \n");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" missing data for constant entry."));
        wrongAddress.clear();
        wrongAddress.str("entry const test");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" is not a valid SNES address."));
        wrongAddress.clear();
        wrongAddress.str("entry const $808000");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains("is larger than the max width for the table."));
        wrongAddress.clear();
        wrongAddress.str("entry const t");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" is not a valid SNES address."));
        t.setStoreWidths(true);
        wrongAddress.clear();
        wrongAddress.str("entry const $8080");
        REQUIRE_THROWS_WITH(t.getDataFromFile(wrongAddress), Contains(" missing width for constant entry."));
        wrongAddress.clear();
        wrongAddress.str("entry const $8080, t");
        REQUIRE_THROWS(t.getDataFromFile(wrongAddress), Contains("is not a decimal or hex number."));
        wrongAddress.clear();
        wrongAddress.str("entry const $8080, $t");
        REQUIRE_THROWS(t.getDataFromFile(wrongAddress), Contains("is not a valid hexadecimal number."));
    }
}
