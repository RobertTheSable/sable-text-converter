#include <catch2/catch.hpp>

#include <sstream>

#include "data/addresslist.h"

using sable::AddressList;

bool operator==(const sable::AddressNode& lhs, const sable::AddressNode& rhs)
{
    return  lhs.address == rhs.address &&
            lhs.label == rhs.label &&
            lhs.isTable == rhs.isTable
    ;
}

template<>
struct Catch::StringMaker<sable::AddressNode>
{
    static std::string convert(const sable::AddressNode& p) {
        std::ostringstream ss;
        ss << (p.isTable ? "Table " : "Address ") << '\"' << (p.label + "\": ")  << std::hex << p.address;
        return ss.str();
    }
};


bool operator==(const sable::TextNode& lhs, const sable::TextNode& rhs)
{
    return  lhs.files == rhs.files &&
            lhs.printpc == rhs.printpc &&
            lhs.size == rhs.size
    ;
}

template<>
struct Catch::StringMaker<sable::TextNode>
{
    static std::string convert(const sable::TextNode& p) {
        std::ostringstream ss;
        ss << '{' << p.files << ' ' << p.size << ' ' << (p.printpc ? "(print)" : "(no print)");
        return ss.str();
    }
};

TEST_CASE("Adding addresses")
{
    AddressList subject;

    subject.addAddress({0, "test 4", false});
    subject.addAddress(5, "test 3", false);
    subject.addAddress({2, "test 2", true});
    subject.addAddress(7, "test 1", true);

    REQUIRE(subject.end() - subject.begin() == 4);
    SECTION("Not sorted.")
    {
        std::vector<sable::AddressNode> expected{
            {0, "test 4", false},
            {5, "test 3", false},
            {2, "test 2", true},
            {7, "test 1", true}
        };
        int index = 0;
        for (auto adr: subject) {
            REQUIRE(index < expected.size());
            REQUIRE(adr == expected[index]);
            ++index;
        }
        REQUIRE(index == 4);
    }
    SECTION("Sorted.")
    {
        subject.sort();
        std::vector<sable::AddressNode> expected{
            {0, "test 4", false},
            {2, "test 2", true},
            {5, "test 3", false},
            {7, "test 1", true}
        };
        int index = 0;
        for (auto adr: subject) {
            REQUIRE(index < expected.size());
            REQUIRE(adr == expected[index]);
            ++index;
        }
        REQUIRE(index == 4);
    }
    SECTION("With Table.")
    {
        sable::Table t(2, false);
        t.setAddress(900);
        t.setDataAddress(1000);
        t.addEntry(910, 5);
        t.addEntry("test entry");

        subject.addTable("test table", std::move(t));
        std::vector<sable::AddressNode> expected{
            {0, "test 4", false},
            {5, "test 3", false},
            {2, "test 2", true},
            {7, "test 1", true},
            {900, "test table", true}
        };
        int index = 0;
        for (auto adr: subject) {
            REQUIRE(index < expected.size());
            REQUIRE(adr == expected[index]);
            ++index;
        }
        REQUIRE(index == 5);
        REQUIRE(subject.checkTable("test table"));
        REQUIRE(subject.getNextAddress("test table") == 1000);
    }
    REQUIRE(!subject.checkTable("not a table"));
    REQUIRE(subject.getNextAddress("") == 0);
}


TEST_CASE("Adding files")
{
    using sable::options::ExportAddress, sable::options::ExportWidth;
    AddressList subject;
    subject.addFile("file_00", "file_00.txt", 5, false, ExportWidth::Off, ExportAddress::On);
    subject.addFile("file_01", {"file_01.txt", 10, false, ExportWidth::Off, ExportAddress::On});
    subject.addFile("file_02", "file_02.txt", 50, true, ExportWidth::Off, ExportAddress::On);
    subject.addFile("file_03", {"file_04.txt", 80, true, ExportWidth::Off, ExportAddress::On});

    struct { //NOLINT
        std::string label;
        sable::TextNode data;
    } cases[] {
        {"file_00", {"file_00.txt", 5, false}},
        {"file_01", {"file_01.txt", 10, false}},
        {"file_02", {"file_02.txt", 50, true}},
        {"file_03", {"file_04.txt", 80, true}}
    };

    for (auto& cs: cases) {
        REQUIRE(subject.getFile(cs.label) == cs.data);
    }
    REQUIRE(subject.begin() == subject.end());
    REQUIRE_THROWS(subject.getFile("filewhichdoesnotexist"));
}

TEST_CASE("next address")
{
    AddressList subject;

    sable::Table t(2, false);
    t.setAddress(900);
    t.setDataAddress(1000);
    t.addEntry(910, 5);
    t.addEntry("test entry");

    subject.addTable("test table", std::move(t));

    REQUIRE(subject.getNextAddress("") == 0);
    REQUIRE(subject.getNextAddress("table which does not exist") == 0);
    REQUIRE(subject.getNextAddress("test table") == 1000);

    subject.setNextAddress(100);
    REQUIRE(subject.getNextAddress("") == 100);
    REQUIRE(subject.getNextAddress("table which does not exist") == 100);
    REQUIRE(subject.getNextAddress("test table") == 1000);

    auto tbl = subject.getTable("test table");
    REQUIRE(tbl.getAddress() == 900);
}
