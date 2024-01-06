#include <catch2/catch.hpp>
#include "yaml-cpp/yaml.h"
#include "project.h"
#include "project/builder.h"

#include "files.h"
#ifdef WIN32
        #define MISSING_FILE_ERROR "sample\\text_map.yml not found."
#else
        #define MISSING_FILE_ERROR "./sample/text_map.yml not found."
#endif

#define MISSING_OR_BAD_FIELD(node, field, value, msg) \
    SECTION("Missing") \
    { \
        node.remove(field); \
    } \
    SECTION("Invalid value") \
    { \
        node[field] = value; \
    } \
    REQUIRE_THROWS_WITH( \
        ProjectSerializer::read(testNode, "."), \
        msg \
    )

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
        SECTION("Invalid (wrong type) ROMs section")
        {
            testNode[Project::ROMS] = "wrong!";
            REQUIRE_THROWS_WITH(
                ProjectSerializer::read(testNode, "."),
                "roms section in config must be a sequence.\n"
            );
        }
        SECTION("Invalid (wrong entry) ROMs section")
        {
            YAML::Node romNode;
            romNode["name"] = "something";
            romNode["file"] = "something.sfc";
            romNode["header"] = "auto";

            YAML::Node romNode2;
            romNode2["name"] = "something2";
            romNode2["file"] = "something2.sfc";
            romNode2["header"] = "false";

            std::string err{""};
            SECTION("bad name")
            {
                SECTION("missing")
                {
                    romNode.remove("name");
                }
                SECTION("invalid")
                {
                    romNode["name"] = std::array{"wrong!"};
                }
                err = "rom at index 1 is missing a name value.\n";
            }
            SECTION("bad file")
            {
                SECTION("missing")
                {
                    romNode.remove("file");
                }
                SECTION("invalid")
                {
                    romNode["file"] = std::array{"wrong!"};
                }
                err = "rom something is missing a file value.\n";
            }
            SECTION("bad header")
            {
                romNode["header"] = "argleblargle";
                err = "rom something does not have a valid header option - must be \"true\", \"false\", \"auto\", or not defined.\n";
            }

            testNode[Project::ROMS] = std::array{romNode2, romNode};
            REQUIRE_THROWS_WITH(
                ProjectSerializer::read(testNode, "."),
                err
            );
        }

        SECTION("Missing Config section")
        {
            testNode.remove(Project::CONFIG_SECTION);
            REQUIRE_THROWS(ProjectSerializer::read(testNode, "."));
        }
        SECTION("Missing config folder.")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::CONFIG_SECTION],
                Project::DIR_VAL,
                std::array{"something"},
                "directory for config section is missing or is not a scalar.\n"
            );
        }
        SECTION("Missing Config files")
        {
            testNode[Project::CONFIG_SECTION].remove(Project::IN_MAP);
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "inMapping for config section is missing.\n");
        }
        SECTION("Invalid Config files")
        {
            testNode[Project::CONFIG_SECTION][Project::IN_MAP] = std::map<std::string, std::string>{{"something", "something"}};
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "inMapping for config section must be a filename or sequence of filenames.\n");
        }

        SECTION("Invalid Config > ROM size")
        {
            SECTION("Wrong type")
            {
                testNode[Project::CONFIG_SECTION][Project::OUT_SIZE] = std::array{"4m"};
                REQUIRE_THROWS_WITH(
                    ProjectSerializer::read(testNode, "."),
                    "config > outputSize must be a string with a valid file size(3mb, 4m, etc)."
                );
            }
            SECTION("Invalid size")
            {
                testNode[Project::CONFIG_SECTION][Project::OUT_SIZE] = "zzzzzzz";
                REQUIRE_THROWS_WITH(
                    ProjectSerializer::read(testNode, "."),
                    "\"zzzzzzz\" is not a supported rom size."
                );
            }
        }

        SECTION("Missing Files section")
        {
            testNode.remove(Project::FILES_SECTION);
            REQUIRE_THROWS(ProjectSerializer::read(testNode, "."));
        }
        SECTION("Invalid Main dir")
        {
            SECTION("Missing")
            {
                testNode[Project::FILES_SECTION].remove(Project::DIR_MAIN);
            }
            SECTION("Not valid")
            {
                testNode[Project::FILES_SECTION][Project::DIR_MAIN] = std::array{"wrong!"};
            }

            REQUIRE_THROWS_WITH(
                ProjectSerializer::read(testNode, "."),
                "main directory for project is missing from files config.\n"
            );
        }

        SECTION("Invalid Input section.")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION],
                Project::INPUT_SECTION,
                "something",
                "input section is missing or not a map.\n"
            );
        }
        SECTION("Invalid Input dir section.")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION][Project::INPUT_SECTION],
                Project::DIR_VAL,
                std::array{"wrong!"},
                "input directory for project is missing from files config.\n"
            );
        }

        SECTION("Invalid Output section.")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION],
                Project::OUTPUT_SECTION,
                "something",
                "output section is missing or not a map.\n"
            );
        }

        SECTION("Invalid Output dir section.")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION],
                Project::DIR_VAL,
                std::array{"wrong!"},
               "output directory for project is missing from files config.\n"
            );
        }

        SECTION("Invalid Output bin section.")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION],
                Project::OUTPUT_BIN,
                "wrong!",
               "output binaries subdirectory section is missing or not a map.\n"
            );
        }

        SECTION("Invalid Output main dir section.")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION][Project::OUTPUT_BIN],
                Project::DIR_MAIN,
                std::array{"wrong!"},
               "output binaries subdirectory mainDir value is missing from files config.\n"
            );
        }

        SECTION("Invalid Output text dir section.")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION][Project::OUTPUT_BIN],
                Project::DIR_TEXT,
                std::array{"wrong!"},
                "output binaries subdirectory textDir value is missing from files config.\n"
            );
        }

        SECTION("Invalid Output extras section.")
        {
            testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION][Project::OUTPUT_BIN][Project::EXTRAS] = "wrong!";
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "extras section for output binaries must be a sequence.\n");
        }

        SECTION("Missing font binaries section")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION][Project::OUTPUT_BIN],
                Project::FONT_SECTION,
                "wrong!",
               "fonts section for output binaries is missing or not a map.\n"
            );
        }
        SECTION("Missing font directory")
        {
            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION][Project::OUTPUT_BIN][Project::FONT_SECTION],
                Project::DIR_FONT,
                std::array{"wrong!"},
                "fonts directory must be a scalar.\n"
            );
        }
        SECTION("Invalid font includes.")
        {
            testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION][Project::OUTPUT_BIN][Project::FONT_SECTION][Project::INCLUDE_VAL] = "wrong!";
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "fonts includes must be a sequence.\n");
        }

        SECTION("Invalid output includes.")
        {
            testNode[Project::FILES_SECTION][Project::OUTPUT_SECTION][Project::INCLUDE_VAL] = "wrong!";
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "includes section for output must be a sequence.\n");
        }

        SECTION("Invalid exportAllAddresses.")
        {
            testNode[Project::CONFIG_SECTION][Project::EXPORT_ALL_ADDRESSES] = std::array{"wrong!"};
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "config > exportAllAddresses must be a string with a valid value(on/off or true/false).\n");
        }

        SECTION("Invalid ROM folder")
        {

            MISSING_OR_BAD_FIELD(
                testNode[Project::FILES_SECTION],
                Project::DIR_ROM,
                std::array{"wrong!"},
                "romDir for project is missing from files config.\n"
            );
        }

        SECTION("Bad locale")
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
    SECTION("Check decoding.")
    {
        caseFileList cs{"sample"};
        cs.add(caseFile{fs::path{"text_map.yml"}, ""});
        // need to set size manually for ToRom method to work
        testNode[Project::CONFIG_SECTION][Project::MAP_TYPE] = "lorom";
        testNode[Project::CONFIG_SECTION][Project::OUT_SIZE] = "4m";
        SECTION("valid mapper settings")
        {
            try {
                SECTION("Mirrored banks on.")
                {
                    testNode[Project::CONFIG_SECTION][Project::USE_MIRRORED_BANKS] = "true";
                    auto pr = ProjectSerializer::read(testNode, ".");
                    REQUIRE(pr.getMapper().ToRom(0) == 0x008000);
                    REQUIRE(pr.getMapper().getType() == sable::util::LOROM);
                }

                SECTION("Mirrored banks off.")
                {
                    testNode[Project::CONFIG_SECTION][Project::USE_MIRRORED_BANKS] = "false";
                    auto pr = ProjectSerializer::read(testNode, ".");
                    REQUIRE(pr.getMapper().ToRom(0) == 0x808000);
                    REQUIRE(pr.getMapper().getType() == sable::util::LOROM);
                }

                SECTION("Type auto-expands.")
                {
                    testNode[Project::CONFIG_SECTION][Project::OUT_SIZE] = "6m";
                    auto pr = ProjectSerializer::read(testNode, ".");
                    REQUIRE(pr.getMapper().getType() == sable::util::EXLOROM);
                }

                SECTION("Type auto-expands with default lorom setting.")
                {
                    testNode[Project::CONFIG_SECTION].remove(Project::MAP_TYPE);
                    testNode[Project::CONFIG_SECTION][Project::OUT_SIZE] = "6m";
                    auto pr = ProjectSerializer::read(testNode, ".");
                    REQUIRE(pr.getMapper().getType() == sable::util::EXLOROM);
                }
            } catch (std::exception& e) {
                FAIL(std::string{"Unexpected exception: "} + e.what());
            }
        }
        SECTION("address export setting enabled")
        {
            testNode[Project::CONFIG_SECTION][Project::EXPORT_ALL_ADDRESSES] = "On";
            auto p = ProjectSerializer::read(testNode, ".");
            REQUIRE(p.areAddressesExported());
        }
        SECTION("address export setting disabled")
        {
            testNode[Project::CONFIG_SECTION][Project::EXPORT_ALL_ADDRESSES] = "Off";
            auto p = ProjectSerializer::read(testNode, ".");
            REQUIRE(!p.areAddressesExported());
        }

        SECTION("Input file options")
        {
            SECTION("only one file")
            {

            }
            SECTION("two files")
            {
                cs.add(caseFile{fs::path{"text_map2.yml"}, ""});
                testNode[Project::CONFIG_SECTION][Project::IN_MAP] = std::array{
                    "text_map.yml",
                    "text_map2.yml"
                };
            }

            REQUIRE_NOTHROW(ProjectSerializer::read(testNode, "."));
        }

        SECTION("Re-write settings")
        {
            auto pr = ProjectSerializer::read(testNode, ".");
            testNode[Project::CONFIG_SECTION].remove(Project::OUT_SIZE);
            ProjectSerializer::write(testNode, pr);
            REQUIRE(testNode[Project::CONFIG_SECTION][Project::OUT_SIZE].Scalar() == "4mb");
        }

        SECTION("invalid mirror option")
        {
            testNode[Project::CONFIG_SECTION][Project::USE_MIRRORED_BANKS] = std::array{"true"};
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "The useMirrorBanks option should be true or false.");
        }
        SECTION("invalid mapper option")
        {
            testNode[Project::CONFIG_SECTION][Project::MAP_TYPE] = "invalid";
            REQUIRE_THROWS_WITH(ProjectSerializer::read(testNode, "."), "The provided mapper must be one of: lorom, hirom, exlorom, exhirom.");
        }
    }
}
