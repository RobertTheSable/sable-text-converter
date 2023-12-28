#include <catch2/catch.hpp>
#include "yaml-cpp/yaml.h"
#include "project.h"
#include "project/builder.h"
#ifdef WIN32
        #define MISSING_FILE_ERROR "sample\\text_map.yml not found."
#else
        #define MISSING_FILE_ERROR "./sample/text_map.yml not found."
#endif

using sable::Project, sable::ProjectSerializer;

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
                    "{directory: sample, inMapping: text_map.yml, locale: en_US.utf8},\n"
                "roms: []"
                "}"
                );
    testNode["roms"].push_back(YAML::Load("{name: test, file: test.smc, header: false}"));
    SECTION("Invalid Configurations")
    {
        SECTION("Empty Node Fails")
        {
            REQUIRE_THROWS(ProjectSerializer::read(YAML::Node(), "."));
        }
        SECTION("Missing ROMs section")
        {
            testNode.remove(Project::ROMS);
            REQUIRE_THROWS(ProjectSerializer::read(testNode, "."));
        }
        SECTION("Missing Config section")
        {
            testNode.remove(Project::CONFIG_SECTION);
            REQUIRE_THROWS(ProjectSerializer::read(testNode, "."));
        }
        SECTION("Missing Files section")
        {
            testNode.remove(Project::FILES_SECTION);
            REQUIRE_THROWS(ProjectSerializer::read(testNode, "."));
        }
        SECTION("Missing font binaries section")
        {
            testNode[Project::FILES_SECTION]
                    [Project::OUTPUT_SECTION]
                    [Project::OUTPUT_BIN].remove(Project::FONT_SECTION);
            REQUIRE_THROWS(ProjectSerializer::read(testNode, "."));
        }
        SECTION("Missing font binaries section")
        {
            testNode[Project::CONFIG_SECTION][Project::LOCALE] = "argleblargleblaaaa";
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "The provided locale is not valid.");
        }
    }
    SECTION("Valid config file throws only exception for missing file.")
    {
        SECTION("Do nothing")
        {

        }
        SECTION("Default locale is used correctly.")
        {
            testNode[Project::CONFIG_SECTION].remove(Project::LOCALE);
        }
        REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), MISSING_FILE_ERROR);
    }
}
