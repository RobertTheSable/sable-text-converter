#include <catch2/catch.hpp>
#include "wrapper/filesystem.h"
#include "yaml-cpp/yaml.h"
#include "project.h"

using sable::Project;

TEST_CASE("Test project config validation", "[project]")
{
    YAML::Node testNode = YAML::Load(
                "{\n"
                "files: {\n"
                    "mainDir: sample,\n"
                    "input: {directory: text},\n"
                    "output: {\n"
                        "directory: asm, includes: [code.asm],\n"
                        "binaries: {\n"
                            "mainDir: bin, textDir: text, extras: [titleScreen],\n"
                            "fonts: {dir: fonts, includes: [fontBinaries] }\n"
                        "}\n"
                    "},\n"
                    "romDir: roms\n"
                "}"
                ", config: \n"
                    "{directory: sample, inMapping: text_map.yml},\n"
                "roms: []"
                "}"
                );
    testNode["roms"].push_back(YAML::Load("{name: test, file: test.smc, header: false}"));

    SECTION("Empty Node Fails")
    {
        REQUIRE_THROWS(Project(YAML::Node(), "."));
    }
    SECTION("Missing ROMs section")
    {
        testNode.remove(Project::ROMS);
        REQUIRE_THROWS(Project(testNode, "."));
    }
    SECTION("Missing Config section")
    {
        testNode.remove(Project::CONFIG_SECTION);
        REQUIRE_THROWS(Project(testNode, "."));
    }
    SECTION("Missing Files section")
    {
        testNode.remove(Project::FILES_SECTION);
        REQUIRE_THROWS(Project(testNode, "."));
    }
    SECTION("Missing font binaries section")
    {
        testNode[Project::FILES_SECTION]
                [Project::OUTPUT_SECTION]
                [Project::OUTPUT_BIN].remove(Project::FONT_SECTION);
        REQUIRE_THROWS(Project(testNode, "."));
    }
    SECTION("Valid config file throws no exceptions.")
    {
        REQUIRE_NOTHROW(Project(testNode, "."));
    }
}
