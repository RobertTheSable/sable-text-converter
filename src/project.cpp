#include "project.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <boost/locale.hpp>
#include "wrapper/filesystem.h"
#include "output/rompatcher.h"
#include "exceptions.h"
#include "localecheck.h"
#include "parse/fileparser.h"
#include "errorhandling.h"
#include "serialize/yamlfontserializer.h"
#include "files/file.h"
#include "files/group.h"
#include "data/addresslist.h"

namespace sable {

Project::Project(const YAML::Node &config, const std::string &projectDir)
    : m_Mapper(util::MapperType::INVALID, false, true), maxAddress(0)
{
    init(config, projectDir);
}

Project::Project(const std::string &projectDir)
    : m_ConfigPath((fs::path(projectDir) / "config.yml").string()), m_Mapper(util::MapperType::INVALID, false, true), maxAddress(0)
{
    if (!fs::exists(fs::path(projectDir) / "config.yml")) {
        throw ConfigError((fs::path(projectDir) / "config.yml").string() + " not found.");
    }
    YAML::Node config = YAML::LoadFile((fs::path(projectDir) / "config.yml").string());
    init(config, projectDir);
}

void Project::init(const YAML::Node &config, const std::string &projectDir)
{
    using std::string;
    using std::vector;
    if (validateConfig(config)) {
        YAML::Node outputConfig = config[FILES_SECTION][OUTPUT_SECTION];
        m_MainDir = config[FILES_SECTION][DIR_MAIN].as<std::string>();
        m_InputDir = config[FILES_SECTION][INPUT_SECTION][DIR_VAL].as<string>();
        m_OutputDir = outputConfig[DIR_VAL].as<string>();
        m_BinsDir = outputConfig[OUTPUT_BIN][DIR_MAIN].as<string>();
        m_TextOutDir = outputConfig[OUTPUT_BIN][DIR_TEXT].as<string>();
        m_FontDir = outputConfig[OUTPUT_BIN][FONT_SECTION][DIR_FONT].as<string>();
        m_RomsDir = config[FILES_SECTION][DIR_ROM].as<string>();
        m_OutputSize = config[CONFIG_SECTION][OUT_SIZE].IsDefined() ?
                    util::calculateFileSize(config[CONFIG_SECTION][OUT_SIZE].as<std::string>()) :
                    0;
        if (config[CONFIG_SECTION][LOCALE].IsDefined()) {
            m_LocaleString = config[CONFIG_SECTION][LOCALE].as<std::string>();
            if (!isLocaleValid(m_LocaleString.c_str())) {
                throw ConfigError("The provided locale is not valid.");
            }
        } else {
            m_LocaleString = "en_US.UTF8";
        }
        if (config[CONFIG_SECTION][MAP_TYPE].IsDefined()) {
            try {
                bool useMirrors = false;
                if (config[CONFIG_SECTION][USE_MIRRORED_BANKS].IsDefined()) {
                    useMirrors = config[CONFIG_SECTION][USE_MIRRORED_BANKS].as<std::string>() == "true";
                }
                auto mapperType = config[CONFIG_SECTION][MAP_TYPE].as<util::MapperType>();
                m_BaseType = mapperType;
                if (m_OutputSize > util::NORMAL_ROM_MAX_SIZE) {
                    mapperType = util::getExpandedType(mapperType);
                }
                m_Mapper = util::Mapper(mapperType, false, !useMirrors, m_OutputSize);
            } catch (YAML::TypedBadConversion<util::MapperType> &e) {
                throw ConfigError("The provided mapper must be one of: lorom, hirom, exlorom, exhirom.");
            } catch (YAML::TypedBadConversion<std::string> &e) {
                throw ConfigError("The useMirrorBanks option should be true or false.");
            }
        } else {
            m_BaseType = util::MapperType::LOROM;
            if (m_OutputSize > util::NORMAL_ROM_MAX_SIZE) {
                m_Mapper = util::Mapper(util::MapperType::EXLOROM, false, true, m_OutputSize);
            } else {
                m_Mapper = util::Mapper(util::MapperType::LOROM, false, true, m_OutputSize);
            }
        }
        fs::path mainDir(projectDir);
        if (fs::path(m_MainDir).is_relative()) {
            m_MainDir = (mainDir / m_MainDir).string();
        }
        if (fs::path(m_RomsDir).is_relative()) {
            m_RomsDir = (mainDir / m_RomsDir).string();
        }
        if (outputConfig[OUTPUT_BIN][FONT_SECTION][INCLUDE_VAL].IsSequence()) {
            for (auto& include : outputConfig[OUTPUT_BIN][FONT_SECTION][INCLUDE_VAL].as<vector<string>>()) {
                m_FontIncludes.push_back(include + ".asm");
            }
        }
        if (outputConfig[OUTPUT_BIN][EXTRAS].IsSequence()) {
            for (auto& include : outputConfig[OUTPUT_BIN][EXTRAS].as<vector<string>>()) {
                m_Extras.push_back((fs::path(include) / (include + ".asm")).string());
            }
        }
        if (outputConfig[INCLUDE_VAL].IsSequence()){
            m_Includes = outputConfig[INCLUDE_VAL].as<vector<string>>();
        }
        m_Roms = config[ROMS].as<vector<Rom>>();

        fs::path fontLocation = mainDir
                / config[CONFIG_SECTION][DIR_VAL].as<string>();

        m_DefaultMode = config[CONFIG_SECTION][DEFAULT_MODE].IsDefined()
                ? config[CONFIG_SECTION][DEFAULT_MODE].as<string>() : "normal";
        if (config[CONFIG_SECTION][IN_MAP].IsScalar()) {
            fontLocation = fontLocation / config[CONFIG_SECTION][IN_MAP].as<std::string>();
            if (!fs::exists(fontLocation)) {
                throw ConfigError(fontLocation.string() + " not found.");
            }
            m_MappingPaths.push_back(fontLocation.string());
        } else if (config[CONFIG_SECTION][IN_MAP].IsSequence()) {
            for (auto&& path: config[CONFIG_SECTION][IN_MAP]) {
                m_MappingPaths.push_back((fontLocation / path.as<std::string>()).string());
            }
        }
    }
}

void outputFile(const std::string &file, const std::vector<unsigned char>& data, size_t length, size_t start)
{
    std::ofstream output(
                file,
                std::ios::binary | std::ios::out
                );
    if (!output) {
        throw ASMError("Could not open " + file + " for writing");
    }
    output.write(reinterpret_cast<const char*>(&data[start]), length);
    output.close();
}

bool Project::parseText()
{
    fs::path mainDir(m_MainDir);
    {
        if (!fs::exists(mainDir / m_OutputDir / m_BinsDir)) {
            if (!fs::exists(mainDir / m_OutputDir)) {
                fs::create_directory(mainDir / m_OutputDir);
            }
            if (!fs::exists(mainDir / m_OutputDir / m_BinsDir)) {
                fs::create_directory(mainDir / m_OutputDir / m_BinsDir);
            }
        } else {
            fs::remove_all(mainDir / m_OutputDir / m_BinsDir / m_TextOutDir);
        }
        fs::create_directory(mainDir / m_OutputDir / m_BinsDir / m_TextOutDir);
    }

//    auto mapperType = m_OutputSize > util::NORMAL_ROM_MAX_SIZE ? util::MapperType::EXLOROM : util::MapperType::LOROM;
//    util::Mapper mapper(mapperType, false, true, m_OutputSize);
    auto locale = getLocale(m_LocaleString);
    FontList fl;
    YamlFontSerializer sz;
    for (auto &path: this->m_MappingPaths) {
        auto inFile = YAML::LoadFile(path);
        for (auto fontIt = inFile.begin(); fontIt != inFile.end(); ++fontIt) {
            fl.AddFont(
                fontIt->first.Scalar(),
                sz.generateFont(
                    fontIt->second,
                    fontIt->first.Scalar(),
                    locale
                )
            );
        }
    }

    FileParser fp {
        .bp = BlockParser{.m_Parser = TextParser(std::move(fl), m_DefaultMode, m_LocaleString)},
        .mapper = m_Mapper
    };
    AddressList addresses;
    //(mainDir / m_OutputDir / m_BinsDir / m_TextOutDir / baseOutputFileName).string()



    {
        fs::path input = fs::path(m_MainDir) / m_InputDir;

        std::vector<fs::path> dirs;
        std::copy(fs::directory_iterator(input), fs::directory_iterator(), std::back_inserter(dirs));
        std::sort(dirs.begin(), dirs.end());
        std::vector<Table> tables;
        std::vector<files::Group<fs::path>> fileGroups;

        for (auto& dir: dirs) {
            files::File fl{dir};
            if (!fl.isDir()) {
                continue;
            }
            auto group = files::Group<fs::path>::buildFromFolder<files::File>(fl, m_Mapper, addresses.getNextAddress(""));
            if (auto tbl = group.getTable(); bool(tbl)) {
                Table cp = tbl.value();
                addresses.addTable(group.getName(), std::move(cp));
            }
            fileGroups.push_back(group);
        }

        int nextAddress = 0;

        auto baseDir = mainDir / m_OutputDir / m_BinsDir / m_TextOutDir;
        auto writer = [baseDir, &addresses] (
                std::string fileName,
                std::string label,
                const std::vector<unsigned char>& data,
                int address,
                size_t start,
                size_t length,
                bool printpc
            ) {
            addresses.addAddress({address, label, false});
            addresses.addFile(label, fileName, length, printpc);
            outputFile((baseDir / fileName).string(), data, length, start);
        };
        for (auto group: fileGroups) {
            int dirIndex = 0;

            auto currentDir = group.getName();

            nextAddress = addresses.getNextAddress(currentDir);

            for (auto &file: group) {
                std::ifstream input(file);
                auto handler = [file] (error::Levels l, std::string msg, int line) {
                    switch (l) {
                    case error::Levels::Error:
                        throw ParseError("Error in text file " + fs::absolute(file).string() + ", line " + std::to_string(line+1) + ": " + msg);
                        break;
                    case error::Levels::Warning:
                        std::cerr <<
                            "Warning in " + fs::absolute(file).string() + " on line " + std::to_string(line) +
                            ": " << msg << "\n";
                        break;
                    default:
                        break;
                    }
                };

                dirIndex = fp.processFile<decltype (handler), decltype (writer)>(input, currentDir, fs::absolute(file).string(), nextAddress, dirIndex, handler, writer);

                input.close();
            } // for (auto &file: group)

            addresses.setNextAddress(nextAddress);
        }
    }
    addresses.sort();

    {
        std::ofstream mainText((mainDir / m_OutputDir / "text.asm").string());
        std::ofstream textDefines((mainDir / m_OutputDir / "textDefines.exp").string());
        if (!mainText || !textDefines) {
            throw ASMError("Could not open files in " + (mainDir / m_OutputDir).string() + " for writing.\n");
        }
        RomPatcher r(m_BaseType);
        r.writeParsedData(addresses, fs::path(m_BinsDir) / m_TextOutDir, mainText, textDefines);
        textDefines.flush();
        textDefines.close();
        mainText.flush();
        mainText.close();
        for (Rom& romData: m_Roms) {
            std::ofstream mainFile((fs::path(m_MainDir) / (romData.name + ".asm")).string());
            mainFile << r.getMapperDirective(m_Mapper.getType()) + "\n\n";
            r.writeIncludes(romData.includes.cbegin(), romData.includes.cend(), mainFile, fs::path(m_OutputDir));
            r.writeIncludes(m_Includes.cbegin(), m_Includes.cend(), mainFile, fs::path(m_OutputDir));
            r.writeIncludes(m_Extras.cbegin(), m_Extras.cend(), mainFile, fs::path(m_OutputDir) / m_BinsDir);
            mainFile << r.generateInclude(fs::path(m_OutputDir) / m_BinsDir / m_FontDir / (m_FontDir + ".asm"), fs::path(), false) + '\n';
            mainFile << r.generateInclude(fs::path(m_OutputDir) / "textDefines.exp", fs::path(), false) + '\n' +
                        r.generateInclude(fs::path(m_OutputDir) / "text.asm", fs::path(), false) + '\n';
            mainFile.close();
        }
        fs::path fontFilePath = fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_FontDir / (m_FontDir + ".asm");
        if (!fs::exists(fontFilePath.parent_path())) {
            fs::create_directories(fontFilePath.parent_path());
        }
        std::ofstream output(fontFilePath.string());
        if (!output) {
            throw ASMError("Could not open " + fontFilePath.string() + " for writing.\n");
        }
        r.writeIncludes(m_FontIncludes.begin(), m_FontIncludes.end(), output);
        r.writeFontData(fp.bp.m_Parser.getFonts(), output);
        output.close();
    }
    maxAddress = (addresses.end()-1)->address;
    return true;
}

void Project::writePatchData()
{
    fs::path mainDir(m_MainDir);

    bool changeSettings = false;
    if (m_OutputSize == 0) {
        m_OutputSize = m_Mapper.calculateFileSize(getMaxAddress());
        changeSettings = true;
    }
    for (Rom& romData: m_Roms) {
        RomPatcher r(m_BaseType);
        std::string patchFile = (mainDir / (romData.name + ".asm")).string();

        fs::path romFilePath = fs::path(m_RomsDir) / romData.file;
        std::string extension = romFilePath.extension().string();
        if (!r.loadRom(romFilePath.string(), romData.name, romData.hasHeader)) {
            std::cerr << fs::absolute(romFilePath).string() + " does not exist, or could not be opened.\n";
        } else {
            if (changeSettings && r.getRealSize() >= m_OutputSize) {
                changeSettings = false;
            }
            r.expand(m_OutputSize, m_Mapper);
            auto result = r.applyPatchFile(patchFile);
            if (result) {
                std::cout << "Assembly for " << romData.name << " completed successfully." << std::endl;
            }
            std::vector<std::string> messages;
            r.getMessages(std::back_inserter(messages));
            if (result) {
                for (auto& msg: messages) {
                    std::cout << msg << std::endl;
                }
                std::ofstream output(
                            (fs::path(m_RomsDir) / (romData.name + extension)).string(),
                            std::ios::out|std::ios::binary
                            );
                output.write(reinterpret_cast<char*>(&r.at(0)), r.getRealSize());
                output.close();
                r.clear();
            } else {
                for (auto& msg: messages) {
                    std::ostringstream error;
                    error << msg << '\n';
                    throw ASMError(error.str());
                }
            }
        }
    }
    if (changeSettings) {
        writeSettings();
    }
}

std::string Project::MainDir() const
{
    return m_MainDir;
}

std::string Project::RomsDir() const
{
    return fs::absolute(m_RomsDir).string();
}

std::string Project::FontConfig() const
{
    return fs::absolute(fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_FontDir).string();
}

std::string Project::TextOutDir() const
{
    return fs::absolute(fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_TextOutDir).string();
}

int Project::getMaxAddress() const
{
    return maxAddress;
}

void Project::writeSettings()
{
    YAML::Node configNode = YAML::LoadFile(m_ConfigPath);
    configNode[CONFIG_SECTION][OUT_SIZE] = util::getFileSizeString(m_OutputSize);
    std::ofstream output(m_ConfigPath);
    if (output) {
        output << configNode << '\n';
    }
    output.close();
}

sable::Project::operator bool() const
{
    return !m_MainDir.empty();
}

bool Project::validateConfig(const YAML::Node &configYML)
{
    std::ostringstream errorString;
    bool isValid = true;
    if (!configYML[FILES_SECTION].IsDefined() || !configYML[FILES_SECTION].IsMap()) {
        isValid = false;
        errorString << "files section is missing or not a map.\n";
    } else {
        if (!configYML[FILES_SECTION][DIR_MAIN].IsDefined()
                || !configYML[FILES_SECTION][DIR_MAIN].IsScalar()) {
            isValid = false;
            errorString << "main directory for project is missing from files config.\n";
        }
        if (!configYML[FILES_SECTION][INPUT_SECTION].IsDefined()
                || !configYML[FILES_SECTION][INPUT_SECTION].IsMap()) {
            isValid = false;
            errorString << "input section is missing or not a map.\n";
        } else {
            if (!configYML[FILES_SECTION][INPUT_SECTION][DIR_VAL].IsDefined()
                    || !configYML[FILES_SECTION][INPUT_SECTION][DIR_VAL].IsScalar()) {
                isValid = false;
                errorString << "input directory for project is missing from files config.\n";
            }
        }
        if (!configYML[FILES_SECTION][OUTPUT_SECTION].IsDefined() || !configYML[FILES_SECTION][OUTPUT_SECTION].IsMap()) {
            isValid = false;
            errorString << "output section is missing or not a map.\n";
        } else {
            YAML::Node outputConfig = configYML[FILES_SECTION][OUTPUT_SECTION];
            if (!outputConfig[DIR_VAL].IsDefined() || !outputConfig[DIR_VAL].IsScalar()) {
                isValid = false;
                errorString << "output directory for project is missing from files config.\n";
            }
            if (!outputConfig[OUTPUT_BIN].IsDefined() || !outputConfig[OUTPUT_BIN].IsMap()) {
                isValid = false;
                errorString << "output binaries subdirectory section is missing or not a map.\n";
            } else {
                if (!outputConfig[OUTPUT_BIN][DIR_MAIN].IsDefined() || !outputConfig[OUTPUT_BIN][DIR_MAIN].IsScalar()) {
                    isValid = false;
                    errorString << "output binaries subdirectory mainDir value is missing from files config.\n";
                }
                if (!outputConfig[OUTPUT_BIN][DIR_TEXT].IsDefined() || !outputConfig[OUTPUT_BIN][DIR_TEXT].IsScalar()) {
                    isValid = false;
                    errorString << "output binaries subdirectory textDir value is missing from files config.\n";
                }
                if (outputConfig[OUTPUT_BIN][EXTRAS].IsDefined() && !outputConfig[OUTPUT_BIN][EXTRAS].IsSequence()) {
                    isValid = false;
                    errorString << "extras section for output binaries must be a sequence.\n";
                }
                if (!outputConfig[OUTPUT_BIN][FONT_SECTION].IsDefined()
                        || !outputConfig[OUTPUT_BIN][FONT_SECTION].IsMap())
                {
                    isValid = false;
                    errorString << "fonts section for output binaries is missing or not a map.\n";
                } else {
                    if (!outputConfig[OUTPUT_BIN][FONT_SECTION][DIR_FONT].IsScalar()) {
                        isValid = false;
                        errorString << "fonts directory must be a scalar.\n";
                    }
                    if (outputConfig[OUTPUT_BIN][FONT_SECTION][INCLUDE_VAL].IsDefined() &&
                        !outputConfig[OUTPUT_BIN][FONT_SECTION][INCLUDE_VAL].IsSequence()
                    ) {
                        isValid = false;
                        errorString << "fonts includes must be a sequence.\n";
                    }
                }
            }
            if (outputConfig[INCLUDE_VAL].IsDefined() && !outputConfig[INCLUDE_VAL].IsSequence()) {
                isValid = false;
                errorString << "includes section for output must be a sequence.\n";
            }
        }
        if (!configYML[FILES_SECTION][DIR_ROM].IsDefined() || !configYML[FILES_SECTION][DIR_ROM].IsScalar()) {
            isValid = false;
            errorString << "romDir for project is missing from files config.\n";
        }
    }
    if (!configYML[CONFIG_SECTION].IsDefined() || !configYML[CONFIG_SECTION].IsMap()) {
        isValid = false;
        errorString << "config section is missing or not a map.\n";
    } else {
        if (!configYML[CONFIG_SECTION][DIR_VAL].IsDefined() || !configYML[CONFIG_SECTION][DIR_VAL].IsScalar()) {
            isValid = false;
            errorString << "directory for config section is missing or is not a scalar.\n";
        }
        if (!configYML[CONFIG_SECTION][IN_MAP].IsDefined() ) {
            isValid = false;
            errorString << "inMapping for config section is missing.\n";
        } else if (configYML[CONFIG_SECTION][IN_MAP].IsMap()) {
            isValid = false;
            errorString << "inMapping for config section must be a filename or sequence of filenames.\n";
        }
//        if (configYML[CONFIG_SECTION][MAP_TYPE].IsDefined() && !configYML[CONFIG_SECTION][MAP_TYPE].IsScalar()) {;
//            isValid = false;
//            errorString << "config > mapper must be a string.\n";
//        }
        if (configYML[CONFIG_SECTION][OUT_SIZE].IsDefined()) {
            if (!configYML[CONFIG_SECTION][OUT_SIZE].IsScalar()) {
                errorString << CONFIG_SECTION + std::string(" > ") + OUT_SIZE +
                               " must be a string with a valid file size(3mb, 4m, etc).";
                isValid = false;
            } else if (util::calculateFileSize(configYML[CONFIG_SECTION][OUT_SIZE].as<std::string>()) == 0) {
                errorString << "\"" + configYML[CONFIG_SECTION][OUT_SIZE].as<std::string>() +
                               "\" is not a supported rom size.";
                isValid = false;
            }
        }
    }
    if (!configYML[ROMS].IsDefined()) {
        isValid = false;
        errorString << "roms section is missing.\n";
    } else {
        if (!configYML[ROMS].IsSequence()) {
            isValid = false;
            errorString << "roms section in config must be a sequence.\n";
        } else {
            int romindex = 0;
            for(auto node: configYML[ROMS]) {
                std::string romName;
                if (!node["name"].IsDefined() || !node["name"].IsScalar()) {
                    errorString << "rom at index " << romindex << " is missing a name value.\n";
                    char temp[50];
                    sprintf(temp, "at index %d", romindex);
                    romName = temp;
                    isValid = false;
                } else {
                    romName = node["name"].Scalar();
                }
                if (!node["file"].IsDefined() || !node["file"].IsScalar()) {
                    errorString << "rom " << romName << " is missing a file value.\n";
                    isValid = false;
                }
                if (node["header"].IsDefined() && (!node["header"].IsScalar() || (
                node["header"].Scalar() != "auto" && node["header"].Scalar() != "true" && node["header"].Scalar() != "false"))) {
                    errorString << "rom " << romName << " does not have a valid header option - must be \"true\", \"false\", \"auto\", or not defined.\n";
                    isValid = false;
                }
            }
        }
    }
    if (!isValid) {
        throw ConfigError(errorString.str());
    }
    return isValid;
}

ConfigError::ConfigError(std::string message) : std::runtime_error(message) {}
ASMError::ASMError(std::string message) : std::runtime_error(message) {}
ParseError::ParseError(std::string message) : std::runtime_error(message) {}

}

bool YAML::convert<sable::Project::Rom>::decode(const Node &node, sable::Project::Rom &rhs)
{
    rhs.name = node["name"].as<std::string>();
    rhs.file = node["file"].as<std::string>();
    rhs.hasHeader = 0;
    if (node["header"].IsDefined()) {
        std::string tmp = node["header"].as<std::string>();
        if (tmp == "true") {
            rhs.hasHeader = 1;
        } else if (tmp == "false") {
            rhs.hasHeader = -1;
        } else if (tmp == "auto") {
            rhs.hasHeader = 0;
        } else {
            return false;
        }
    }
    if (node["includes"].IsDefined() && node["includes"].IsSequence()) {
        rhs.includes = node["includes"].as<std::vector<std::string>>();
    }
    return true;
}
