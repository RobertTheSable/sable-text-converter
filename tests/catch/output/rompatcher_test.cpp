#include <catch2/catch.hpp>
#include "output/rompatcher.h"

#include <cstring>
#include <fstream>

#include "helpers.h"
#include "font/builder.h"
#include "data/options.h"
#include "project/project.h"

using sable::RomPatcher;

auto getLines(std::string& data)
{
    std::vector<std::string> lines;
    std::size_t lastPos = 0u;
    for (auto pos = data.find("\n", lastPos); pos != std::string::npos; pos = data.find("\n", lastPos)) {
        if (pos == lastPos) {
            ++lastPos;
            lines.push_back("");
            continue;
        }
        auto tmp = data.substr(lastPos, pos);
        if (auto npos = tmp.find("\n"); npos != std::string::npos) {
            tmp.erase(npos);
        }
        lines.push_back(tmp);
        lastPos = pos+1;
    }
    return lines;
}

TEST_CASE("Writing includes", "[rompatcher]")
{
    std::vector<std::string> includes = {"something", "something2", "something3"};
    std::ostringstream sink;
    RomPatcher r;

    r.writeIncludes(includes.cbegin(), includes.cend(), sink, fs::path("somewhere"));

    std::string data = sink.str();
    auto lines = getLines(data);
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "incsrc somewhere/something");
    REQUIRE(lines[1] == "incsrc somewhere/something2");
    REQUIRE(lines[2] == "incsrc somewhere/something3");
}

TEST_CASE("Writing single include", "[rompatcher]")
{
    std::string include = "something";
    std::ostringstream sink;
    RomPatcher r;

    r.writeInclude(include, sink, fs::path("somewhere"));

    std::string data = sink.str();
    auto lines = getLines(data);
    REQUIRE(lines.size() == 1);
    REQUIRE(lines[0] == "incsrc somewhere/something");
}

TEST_CASE("Mapper decoding.", "[rompatcher]")
{
    using sable::util::MapperType;
    RomPatcher r;
    REQUIRE(r.getMapperDirective(MapperType::LOROM) == "lorom");
    REQUIRE(r.getMapperDirective(MapperType::HIROM) == "hirom");
    REQUIRE(r.getMapperDirective(MapperType::EXLOROM) == "exlorom");
    REQUIRE(r.getMapperDirective(MapperType::EXHIROM) == "exhirom");
    REQUIRE_THROWS(r.getMapperDirective(MapperType::INVALID));
}

TEST_CASE("Rom loading.", "[rompatcher]")
{
    RomPatcher r(sable::util::MapperType::LOROM);

    SECTION("Valid ROMS")
    {
        SECTION("Rom with no header")
        {
            r.loadRom("sample.sfc", "", -1);
            REQUIRE(r.getRealSize() == 131072);
        }
        SECTION("Rom with a header")
        {
            r.loadRom("sample.smc", "", 1);
            REQUIRE(r.getRealSize() == 131584);
        }
        SECTION("Rom with no header, autodetection")
        {
            r.loadRom("sample.sfc", "", 0);
            REQUIRE(r.getRealSize() == 131072);
        }
        SECTION("Rom with a header, autodetection")
        {
            r.loadRom("sample.smc", "", 0);
            REQUIRE(r.getRealSize() == 131584);
        }
        r.clear();
        REQUIRE(r.getRealSize() == 0);
    }
}

TEST_CASE("Expansion Test", "[rompatcher]")
{
    using sable::RomPatcher;
    SECTION("LoROM")
    {
        RomPatcher r(sable::util::MapperType::LOROM);
        sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
        SECTION("Resizing to normal ROM size.")
        {
            r.loadRom("sample.sfc", "patch test", -1);
            REQUIRE(r.getRealSize() == 131072);

            REQUIRE_NOTHROW(r.expand(m.calculateFileSize(0xE08001), m));
            REQUIRE(r.getRealSize() == 0x380000);
        }
        SECTION("Resizing to EXLOROM ROM size.")
        {
            SECTION("Base LOROM mapper.")
            {
                r = RomPatcher(sable::util::MapperType::LOROM);
            }
            SECTION("Corrected base size from EXLOROM > LOROM.")
            {
                r = RomPatcher(sable::util::MapperType::EXLOROM);
            }
            r.loadRom("sample.sfc", "patch test", -1);
            sable::util::Mapper m2(sable::util::MapperType::EXLOROM, false, true, 0x600000);
            REQUIRE(m.ToPC(sable::util::HEADER_LOCATION) == 0x007FC0);
            unsigned char test = r.at(m.ToPC(sable::util::HEADER_LOCATION));
            unsigned char mode = r.at(m.ToPC(sable::util::HEADER_LOCATION) + 0x15);
            REQUIRE_NOTHROW(r.expand(m2.calculateFileSize(0x208000), m2));
            REQUIRE(r.getRealSize() == 0x600000);
            REQUIRE(m2.ToPC(sable::util::HEADER_LOCATION) == 0x400000+m.ToPC(sable::util::HEADER_LOCATION));
            REQUIRE(r.at(m2.ToPC(sable::util::HEADER_LOCATION)) == test);
            REQUIRE(r.at(m2.ToPC(sable::util::HEADER_LOCATION) + 0x15) == mode);
        }
    }
    SECTION("Resizing to EXHIROM ROM size.")
    {
        RomPatcher r(sable::util::MapperType::HIROM);
        SECTION("Base HIROM mapper.")
        {
            r = RomPatcher(sable::util::MapperType::HIROM);
        }
        SECTION("Corrected base size from EXHIROM > HIROM.")
        {
            r = RomPatcher(sable::util::MapperType::EXHIROM);
        }

        sable::util::Mapper m(sable::util::MapperType::HIROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
        sable::util::Mapper m2(sable::util::MapperType::EXHIROM, false, true, 0x600000);
        r.loadRom("sample.sfc", "patch test", -1);

        REQUIRE(m.ToPC(sable::util::HEADER_LOCATION) == 0x00FFC0);
        REQUIRE(m2.ToPC(sable::util::HEADER_LOCATION) == 0x400000+m.ToPC(sable::util::HEADER_LOCATION));

        unsigned char test = r.at(m.ToPC(sable::util::HEADER_LOCATION));
        unsigned char mode = r.at(m.ToPC(sable::util::HEADER_LOCATION) + 0x15);
        r.expand(m2.calculateFileSize(0x408000), m2);
        REQUIRE(r.getRealSize() == 0x600000);
        REQUIRE(r.at(m2.ToPC(sable::util::HEADER_LOCATION)) == test);
        REQUIRE(r.at(m2.ToPC(sable::util::HEADER_LOCATION) + 0x15) == (mode | 4) );
    }
}

TEST_CASE("Expansion error checking.", "[rompatcher]")
{
    SECTION("HIROM > EXLOROM throws logic error")
    {
        RomPatcher r(sable::util::MapperType::HIROM);
        sable::util::Mapper m(sable::util::MapperType::EXLOROM, false, true, 0x600000);
        REQUIRE_THROWS_AS(r.expand(0x60000, m), std::logic_error);
    }
    SECTION("LoROM > EXHiROM throws logic error")
    {
        RomPatcher r(sable::util::MapperType::LOROM);
        sable::util::Mapper m(sable::util::MapperType::EXHIROM, false, true, 0x600000);
        REQUIRE_THROWS_AS(r.expand(0x60000, m), std::logic_error);
    }
    SECTION("Non-Expanded ROM throws logic error")
    {
        RomPatcher r(sable::util::MapperType::LOROM);
        r.loadRom("sample.sfc", "patch test", -1);
        sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, 0x400000);
        REQUIRE_THROWS_AS(r.expand(0x600000, m), std::logic_error);
    }
    SECTION("Too large size throws runtime error")
    {
        RomPatcher r(sable::util::MapperType::HIROM);
        sable::util::Mapper m(sable::util::MapperType::EXHIROM, false, true, 0x600000);
        REQUIRE_THROWS_AS(r.expand(0x60000000, m), std::runtime_error);
    }
}


TEST_CASE("Asar patch testing", "[rompatcher]")
{
    using sable::RomPatcher;
    RomPatcher r(sable::util::MapperType::LOROM);
    sable::util::Mapper m(sable::util::MapperType::LOROM, false, true, sable::util::NORMAL_ROM_MAX_SIZE);
    r.loadRom("sample.sfc", "patch test", -1);
    r.expand(m.calculateFileSize(0xE08000), m);
    SECTION("Test that Asar patch can be applied.")
    {
        REQUIRE(RomPatcher::succeeded(r.applyPatchFile("sample.asm")));
        std::vector<std::string> msgs;
        REQUIRE(r.getMessages(std::back_inserter(msgs)));
        REQUIRE(msgs.empty());
    }
    SECTION("Test bad Assar patch.")
    {
        REQUIRE(!RomPatcher::succeeded(r.applyPatchFile("bad_test.asm")));
        std::vector<std::string> msgs;
        REQUIRE(r.getMessages(std::back_inserter(msgs)));
        REQUIRE(!msgs.empty());
    }
}

TEST_CASE("Asar status evaluation", "[rompatcher]")
{
    using sable::RomPatcher;
    using State = sable::RomPatcher::AsarState;
    REQUIRE(RomPatcher::succeeded(State::Success));
    REQUIRE(!RomPatcher::succeeded(State::Error));
    REQUIRE(!RomPatcher::succeeded(State::NotRun));
    REQUIRE(!RomPatcher::succeeded(State::InitFailed));
    REQUIRE(RomPatcher::wasRun(State::Success));
    REQUIRE(RomPatcher::wasRun(State::Error));
    REQUIRE(!RomPatcher::wasRun(State::NotRun));
    REQUIRE(!RomPatcher::wasRun(State::InitFailed));
}

TEST_CASE("Font output", "[rompatcher]")
{
    auto fl = sable_tests::getSampleNode();

    fl["normal"][sable::Font::ENCODING] = {};
    for (int i = 0; i < 26; ++i) {
        YAML::Node n;

        char c = 'A' + i;
        n[sable::Font::CODE_VAL] = i+1;
        n[sable::Font::TEXT_LENGTH_VAL] = 8;
        fl["normal"][sable::Font::ENCODING][std::string(1, c)] = n;
    }
    fl["normal"][sable::Font::MAX_CHAR] = 26;
    fl["normal"][sable::Font::DEFAULT_WIDTH] = 0;
    fl["normal"].remove(sable::Font::PAGES);

    std::map<std::string, sable::Font> subject1{};

    std::ostringstream sink;
    RomPatcher r(sable::util::MapperType::LOROM);
    SECTION("Font with no location.")
    {
        subject1["test"] = sable::FontBuilder::make(
            fl["normal"],
            "test",
            sable_tests::defaultLocale
        );
        r.writeFontData(subject1, sink);
        REQUIRE(sink.str() == "");
    }
    SECTION("Font with a location.")
    {
        fl["normal"][sable::Font::FONT_ADDR] = "!somewhere";
        subject1["test"] = sable::FontBuilder::make(
            fl["normal"],
            "test",
            sable_tests::defaultLocale
        );
        r.writeFontData(subject1, sink);
        REQUIRE(sink.str() != "");

        std::string data = sink.str();
        auto lines = getLines(data);

        REQUIRE(lines.size() == 4);
        REQUIRE(lines[0] == "");
        REQUIRE(lines[1] == "ORG !somewhere");
        REQUIRE(lines[2] == "db $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08");
        REQUIRE(lines[3] == "db $08, $08, $08, $08, $08, $08, $08, $08, $08, $08");
    }
    SECTION("Font with skipped data.")
    {
        fl["normal"][sable::Font::FONT_ADDR] = "!somewhere";
        fl["normal"][sable::Font::ENCODING].remove("C");
        fl["normal"][sable::Font::ENCODING].remove("D");

        subject1["test"] = sable::FontBuilder::make(
            fl["normal"],
            "test",
            sable_tests::defaultLocale
        );
        r.writeFontData(subject1, sink);
        REQUIRE(sink.str() != "");

        std::string data = sink.str();
        auto lines = getLines(data);

        REQUIRE(lines.size() == 6);
        REQUIRE(lines[0] == "");
        REQUIRE(lines[1] == "ORG !somewhere");
        REQUIRE(lines[2] == "db $08, $08");
        REQUIRE(lines[3] == "skip 2");
        REQUIRE(lines[4] == "db $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08");
        REQUIRE(lines[5] == "db $08, $08, $08, $08, $08, $08");
    }
    SECTION("Font with a glyph alias.")
    {
        fl["normal"][sable::Font::FONT_ADDR] = "!somewhere";
        fl["normal"][sable::Font::ENCODING]["Z"][sable::Font::CODE_VAL] = 25;
        fl["normal"][sable::Font::MAX_CHAR] = 25;
        subject1["test"] = sable::FontBuilder::make(
            fl["normal"],
            "test",
            sable_tests::defaultLocale
        );
        r.writeFontData(subject1, sink);
        REQUIRE(sink.str() != "");

        std::string data = sink.str();
        auto lines = getLines(data);

        REQUIRE(lines.size() == 4);
        REQUIRE(lines[0] == "");
        REQUIRE(lines[1] == "ORG !somewhere");
        REQUIRE(lines[2] == "db $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08");
        REQUIRE(lines[3] == "db $08, $08, $08, $08, $08, $08, $08, $08, $08");
    }
}

TEST_CASE("Address data output", "[rompatcher]")
{
    using sable::options::ExportAddress, sable::options::ExportWidth;
    sable::RomPatcher r;
    sable::AddressList al;
    std::ostringstream defineSink, textSink;
    fs::path writeDir("test");
    al.addFile("somefile", "file.bin", 16, false, ExportWidth::Off, ExportAddress::On);
    SECTION("File include.")
    {
        al.addAddress(0x908000, "somefile", false);
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_somefile = $908000\n");

        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 4);
        REQUIRE(lines[0] == "ORG !def_somefile");
        REQUIRE(lines[1] == "somefile:");
        REQUIRE(lines[2] == "incbin test/file.bin");
        REQUIRE(lines[3] == "");
    }
    SECTION("File include with program counter printed.")
    {
        al.addFile("somefile", "file.bin", 16, true, ExportWidth::Off, ExportAddress::On);
        al.addAddress(0x908000, "somefile", false);
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_somefile = $908000\n");

        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 5);
        REQUIRE(lines[0] == "ORG !def_somefile");
        REQUIRE(lines[1] == "somefile:");
        REQUIRE(lines[2] == "incbin test/file.bin");
        REQUIRE(lines[3] == "print pc");
    }
    SECTION("File include with width exported.")
    {
        al.addFile("somefile", "file.bin", 16, true, ExportWidth::On, ExportAddress::On);
        al.addAddress(0x908000, "somefile", false);
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_somefile = $908000\n!def_somefile_length = 16\n");

        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 5);
        REQUIRE(lines[0] == "ORG !def_somefile");
        REQUIRE(lines[1] == "somefile:");
        REQUIRE(lines[2] == "incbin test/file.bin");
        REQUIRE(lines[3] == "print pc");
    }
    SECTION("File include with no label.")
    {
        al.addFile("$somefile", "file2.bin", 16, false, ExportWidth::Off, ExportAddress::On);
        al.addAddress(0x908000, "somefile", false);
        al.addAddress(0x90F000, "$somefile", false);
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_somefile = $908000\n");

        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 7);
        REQUIRE(lines[0] == "ORG !def_somefile");
        REQUIRE(lines[1] == "somefile:");
        REQUIRE(lines[2] == "incbin test/file.bin");
        REQUIRE(lines[3] == "");

        REQUIRE(lines[4] == "ORG $90f000");
        REQUIRE(lines[5] == "incbin test/file2.bin");
        REQUIRE(lines[6] == "");
    }
    SECTION("Second file include with explicit address.")
    {
        al.addFile("somefile2", "file2.bin", 16, false, ExportWidth::Off, ExportAddress::On);
        al.addAddress(0x908000, "somefile", false);
        al.addAddress(0x908010, "somefile2", false);
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_somefile = $908000\n"
                                    "!def_somefile2 = $908010\n");

        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 8);
        REQUIRE(lines[0] == "ORG !def_somefile");
        REQUIRE(lines[1] == "somefile:");
        REQUIRE(lines[2] == "incbin test/file.bin");
        REQUIRE(lines[3] == "");
        REQUIRE(lines[4] == "ORG !def_somefile2");
        REQUIRE(lines[5] == "somefile2:");
        REQUIRE(lines[6] == "incbin test/file2.bin");
        REQUIRE(lines[7] == "");
    }
    SECTION("Second file include with implicit address.")
    {
        al.addFile("somefile2", "file2.bin", 16, false, ExportWidth::Off, ExportAddress::Off);
        al.addAddress(0x908000, "somefile", false);
        al.addAddress(0x908010, "somefile2", false);
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_somefile = $908000\n");

        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 7);
        REQUIRE(lines[0] == "ORG !def_somefile");
        REQUIRE(lines[1] == "somefile:");
        REQUIRE(lines[2] == "incbin test/file.bin");
        REQUIRE(lines[3] == "");
        REQUIRE(lines[4] == "somefile2:");
        REQUIRE(lines[5] == "incbin test/file2.bin");
        REQUIRE(lines[6] == "");
    }
    SECTION("Table with 3-byte addresses")
    {
        sable::Table tbl;
        tbl.setAddress(0x808000);
        tbl.addEntry("somefile");
        tbl.addEntry(0xe08000, 16);
        tbl.setAddressSize(3);
        al.addTable("somewhere", std::move(tbl));
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_table_somewhere = $808000\n");
        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 5);
        REQUIRE(lines[0] == "ORG !def_table_somewhere");
        REQUIRE(lines[1] == "table_somewhere:");
        REQUIRE(lines[2] == "dl somefile");
        REQUIRE(lines[3] == "dl $e08000");
        REQUIRE(lines[4] == "");
    }
    SECTION("Table with 3-byte addresses")
    {
        sable::Table tbl;
        tbl.setAddress(0x808000);
        tbl.addEntry("somefile");
        tbl.addEntry(0xe08000, 16);
        tbl.setAddressSize(2);
        al.addTable("somewhere", std::move(tbl));
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_table_somewhere = $808000\n");
        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 5);
        REQUIRE(lines[0] == "ORG !def_table_somewhere");
        REQUIRE(lines[1] == "table_somewhere:");
        REQUIRE(lines[2] == "dw somefile");
        REQUIRE(lines[3] == "dw $8000");
        REQUIRE(lines[4] == "");
    }
    SECTION("Table with 3-byte addresses and sizes")
    {
        sable::Table tbl;
        tbl.setStoreWidths(true);
        tbl.setAddress(0x808000);
        tbl.addEntry("somefile");
        tbl.addEntry(0xe08000, 16);
        tbl.setAddressSize(3);
        al.addTable("somewhere", std::move(tbl));
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_table_somewhere = $808000\n");
        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 5);
        REQUIRE(lines[0] == "ORG !def_table_somewhere");
        REQUIRE(lines[1] == "table_somewhere:");
        REQUIRE(lines[2] == "dl somefile, 16");
        REQUIRE(lines[3] == "dl $e08000, 16");
        REQUIRE(lines[4] == "");
    }
    SECTION("Table with 3-byte addresses and sizes")
    {
        sable::Table tbl;
        tbl.setStoreWidths(true);
        tbl.setAddress(0x808000);
        tbl.addEntry("somefile");
        tbl.addEntry(0xe08000, 16);
        tbl.setAddressSize(2);
        al.addTable("somewhere", std::move(tbl));
        r.writeParsedData(al, writeDir, textSink, defineSink);
        REQUIRE(defineSink.str() == "!def_table_somewhere = $808000\n");
        std::string data = textSink.str();
        auto lines = getLines(data);
        REQUIRE(lines.size() == 5);
        REQUIRE(lines[0] == "ORG !def_table_somewhere");
        REQUIRE(lines[1] == "table_somewhere:");
        REQUIRE(lines[2] == "dw somefile, 16");
        REQUIRE(lines[3] == "dw $8000, 16");
        REQUIRE(lines[4] == "");
    }
    SECTION("Table with invalid address size")
    {
        sable::Table tbl;
        tbl.setStoreWidths(true);
        tbl.setAddress(0x808000);
        tbl.addEntry("somefile");
        tbl.addEntry(0xe08000, 16);
        tbl.setAddressSize(4);
        al.addTable("somewhere", std::move(tbl));
        REQUIRE_THROWS(r.writeParsedData(al, writeDir, textSink, defineSink));
    }
}
