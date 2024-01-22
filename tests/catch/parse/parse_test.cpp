#include <catch2/catch.hpp>

#include <sstream>

#include "parse/parse.h"

#include "helpers.h"

using sable_tests::getSampleFonts;

struct stubWriter: sable::Parser<stubWriter>
{
    struct expected
    {
        std::string fileName;
        std::string label;
        std::vector<unsigned char> data;
        int address;
        size_t start;
        size_t length;
        bool printpc;
    };
    struct errorParams
    {
        std::string file;
        sable::error::Levels l;
        std::string msg;
        int line;
    };
    std::vector<expected> cases;
    std::vector<errorParams> errors;

    stubWriter(const std::string& defaultMode)
        : Parser(getSampleFonts(), defaultMode, "en_US.utf-8", sable::options::ExportWidth::Off, sable::options::ExportAddress::On)
    {
    }

    void report(
        std::string file,
        sable::error::Levels l,
        std::string msg,
        int line
    ) {
        errors.push_back({file, l, msg, line});
        if (l == sable::error::Levels::Error) {
            throw std::runtime_error("Test Halted");
        }
    }

    void write (
        std::string fileName,
        std::string label,
        const std::vector<unsigned char>& data,
        int address,
        size_t start,
        size_t length,
        bool printpc,
        sable::options::ExportWidth,
        sable::options::ExportAddress
    )
    {
        cases.push_back({fileName, label, data, address, start, length, printpc});
    }
};

TEST_CASE("Loading script data from files")
{
    stubWriter subject("normal");;
    std::istringstream input;
    sable::util::Mapper m(sable::util::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    sable::parse::FileResult r;
    SECTION("Basic file")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));
        REQUIRE(r.dirIndex == 0);
        REQUIRE(r.address == 0x808009);
        REQUIRE(subject.cases.size() == 1);
        REQUIRE(subject.cases[0].data.size() == 9);
        REQUIRE(subject.cases[0].length == 9);
        REQUIRE(subject.cases[0].fileName == "test1.bin");
        REQUIRE(subject.cases[0].label == "test1");
        REQUIRE(subject.cases[0].address == 0x808000);
        REQUIRE(subject.cases[0].start == 0);
        REQUIRE(subject.cases[0].printpc == 0);
    }
    SECTION("File that crosses banks")
    {
        input.str("@address 80FFFF\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));
        REQUIRE(subject.cases.size() == 2);
        REQUIRE(subject.cases[0].fileName == "test1bank.bin");
        REQUIRE(subject.cases[0].label == "$test1");
        REQUIRE(subject.cases[0].address == 0x818000);
        REQUIRE(subject.cases[0].data.size() == 9);
        REQUIRE(subject.cases[0].length == 8);

        REQUIRE(subject.cases[1].fileName == "test1.bin");
        REQUIRE(subject.cases[1].label == "test1");
        REQUIRE(subject.cases[1].address == 0x80FFFF);
        REQUIRE(subject.cases[1].data.size() == 9);
        REQUIRE(subject.cases[1].length == 1);

        REQUIRE(r.address == 0x818008);
        REQUIRE(r.dirIndex == 0);
    }
    SECTION("File with multiple pieces of text")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG[End]\n"
                  "@label test2\n"
                  "ABCDEFG\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));
        REQUIRE(subject.cases.size() == 2);
        REQUIRE(subject.cases[0].address == 0x808000);
        REQUIRE(subject.cases[0].label == "test1");
        REQUIRE(subject.cases[0].data.size() == 9);
        REQUIRE(subject.cases[0].length == 9);

        REQUIRE(subject.cases[1].address == 0x808009);
        REQUIRE(subject.cases[1].label == "test2");
        REQUIRE(subject.cases[1].data.size() == 9);
        REQUIRE(subject.cases[1].length == 9);

        REQUIRE(r.address == 0x808012);
        REQUIRE(r.dirIndex == 0);
    }
    SECTION("File with no label")
    {
        input.str("@address 808000\n"
                  "ABCDEFG[End]\n"
                  "ABCDEFG\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));
        REQUIRE(r.address == 0x808012);
        REQUIRE(r.dirIndex == 2);

        REQUIRE(subject.cases.size() == 2);
        REQUIRE(subject.cases[0].address == 0x808000);
        REQUIRE(subject.cases[0].label == "test_0");
        REQUIRE(subject.cases[0].data.size() == 9);
        REQUIRE(subject.cases[0].length == 9);

        REQUIRE(subject.cases[1].address == 0x808009);
        REQUIRE(subject.cases[1].label == "test_1");
        REQUIRE(subject.cases[1].data.size() == 9);
        REQUIRE(subject.cases[1].length == 9);

    }
    SECTION("File ending with a comment")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n"
                  "XYZ\n"
                  "#comment\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));
        REQUIRE(subject.cases.size() == 1);
        REQUIRE(subject.cases[0].address == 0x808000);
        REQUIRE(subject.cases[0].label == "test1");
        REQUIRE(subject.cases[0].data.size() == 14);
        REQUIRE(subject.cases[0].length == 14);

        REQUIRE(r.address == 0x80800E);
        REQUIRE(r.dirIndex == 0);
    }
    SECTION("Parse two files from the same path")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));

        input.clear();
        input.str("@label test2\n"
                  "TUVWXYZ\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", r.address, r.dirIndex));
        REQUIRE(subject.cases.size() == 2);
        REQUIRE(subject.cases[0].address == 0x808000);
        REQUIRE(subject.cases[0].label == "test1");

        REQUIRE(subject.cases[1].address == 0x808009);
        REQUIRE(subject.cases[1].label == "test2");

        REQUIRE(r.address == 0x808012);
        REQUIRE(r.dirIndex == 0);
    }
    REQUIRE(subject.errors.size() == 0);
}

TEST_CASE("Parse with an alternate default mode")
{
    stubWriter subject("menu");;
    std::istringstream input;
    sable::util::Mapper m(sable::util::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    sable::parse::FileResult r;

    input.str("@address 808000\n"
              "@label test1\n"
              "ABCDEFG\n");
    REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));
    REQUIRE(subject.cases.size() == 1);
    REQUIRE(subject.cases[0].length == 16);
    REQUIRE(subject.cases[0].data.size() == 16);
    REQUIRE(subject.cases[0].data.back() == 0xFF);
}

TEST_CASE("Data store error handling")
{

    stubWriter subject("normal");;
    std::istringstream input;
    sable::util::Mapper m(sable::util::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    sable::parse::FileResult r;
    SECTION("Long line detection")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));
        REQUIRE(subject.errors.size() == 1);
        REQUIRE(subject.errors[0].l == sable::error::Levels::Warning);
        REQUIRE(subject.errors[0].file == "test/test.txt");
        REQUIRE(subject.errors[0].line == 3);
        REQUIRE(subject.errors[0].msg == "Line is longer than the specified max width of 160 pixels.");
        REQUIRE(subject.cases.size() == 1);
        REQUIRE(subject.cases[0].length == 95);
        REQUIRE(subject.cases[0].data.size() == 95);
        REQUIRE(r.dirIndex == 0);

    }
    SECTION("File which causes a collision")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "ABCDEFG\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test.txt", 0, 0));

        input.clear();
        input.str("@address 808005\n"
                  "@label test2\n"
                  "TUVWXYZ\n");
        REQUIRE_NOTHROW(r = subject.processFile(input, m, "test", "test/test2.txt", r.address, r.dirIndex));
        REQUIRE(subject.cases.size() == 2);
        REQUIRE(subject.cases[0].address == 0x808000);
        REQUIRE(subject.cases[0].label == "test1");

        REQUIRE(subject.cases[1].address == 0x808005);
        REQUIRE(subject.cases[1].label == "test2");

        REQUIRE(subject.errors.size() == 1);
        REQUIRE(subject.errors[0].l == sable::error::Levels::Warning);
        REQUIRE(subject.errors[0].file == "test/test2.txt");
        REQUIRE(subject.errors[0].line == 3);
        REQUIRE(subject.errors[0].msg == "block \"test2\" collides with block \"test1\" from file \"test/test.txt\".");
    }
    SECTION("Invalid file handling")
    {
        input.str("@address 808000\n"
                  "@label test1\n"
                  "%");
        try {
            subject.processFile(input, m, "test", "test/test.txt", 0, 0);
            FAIL("Error was not detected.");
        }  catch (std::runtime_error &e) {
            REQUIRE(subject.errors.size() == 1);
            REQUIRE(subject.cases.size() == 0);
            REQUIRE(subject.errors[0].l == sable::error::Levels::Error);
            REQUIRE(subject.errors[0].file == "test/test.txt");
            REQUIRE(subject.errors[0].line == 3);
            REQUIRE(subject.errors[0].msg == "\"%\" not found in Encoding of font normal");
        }
    }
    SECTION("Invalid address handling")
    {
        input.str("@label test1\n"
                  "ABCDEFG\n");
        try {
            subject.processFile(input, m, "test", "test/test.txt", 0x7e0000, 0);
            FAIL("Error was not detected.");
        }  catch (std::runtime_error &e) {
            REQUIRE(subject.errors.size() == 1);
            REQUIRE(subject.cases.size() == 0);
            REQUIRE(subject.errors[0].l == sable::error::Levels::Error);
            REQUIRE(subject.errors[0].file == "test/test.txt");
            REQUIRE(subject.errors[0].line == 2);
            REQUIRE(subject.errors[0].msg == "Attempted to begin parsing with invalid ROM address $7e0000");
        }
    }
}
