#include "project.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "wrapper/filesystem.h"
#include "util.h"
#include "rompatcher.h"
#include "exceptions.h"
#include "datastore.h"

namespace sable {

Project::Project(const YAML::Node &config, const std::string &projectDir) : nextAddress(0), maxAddress(0)
{
    init(config, projectDir);
}

Project::Project(const std::string &projectDir) : nextAddress(0), maxAddress(0)
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
                / config[CONFIG_SECTION][DIR_VAL].as<string>()
                / config[CONFIG_SECTION][IN_MAP].as<string>();
        m_DefaultMode = config[CONFIG_SECTION][DEFAULT_MODE].IsDefined()
                ? config[CONFIG_SECTION][DEFAULT_MODE].as<string>() : "normal";
        if (!fs::exists(fontLocation)) {
            throw ConfigError(fontLocation.string() + " not found.");
        }
        m_FontConfigPath = fontLocation.string();
    }
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

    DataStore m_DataStore = DataStore(YAML::LoadFile(m_FontConfigPath), m_DefaultMode);
    {
        fs::path input = fs::path(m_MainDir) / m_InputDir;
        std::vector<std::string> allFiles;
        std::vector<fs::path> dirs;
        std::copy(fs::directory_iterator(input), fs::directory_iterator(), std::back_inserter(dirs));
        std::sort(dirs.begin(), dirs.end());
        for (auto& dir: dirs) {
            if (fs::is_directory(dir)) {
                std::vector<std::string> files;
                if (fs::exists(dir / "table.txt")) {
                    std::string path = (dir / "table.txt").string();
                    std::ifstream tablefile(path);
                    if (tablefile) {
                        files = m_DataStore.addTable(tablefile, (dir / "table.txt"));
                    } else {
                        throw ParseError(fs::absolute(path).string() + " could not be opened.");
                    }
                    tablefile.close();
                    for (std::string& file: files) {
                        if (!fs::exists(dir / file)) {
                            std::cerr << "In " + path
                                         + ": file " + fs::absolute(dir / file).string()
                                         + " does not exist." << std::endl;
                        }
                        file = (dir / file).string();
                    }
                } else {
                    for (auto& fileIt: fs::directory_iterator(dir)) {
                        if (fs::is_regular_file(fileIt.path())) {
                            files.push_back(fileIt.path().string());
                        }
                    }
                }
                std::sort(files.begin(), files.end());
                allFiles.insert(allFiles.end(), files.begin(), files.end());
            }
        }
        for (auto &file: allFiles) {
            std::ifstream input(file);
            while (input) {
                m_DataStore.addFile(input, fs::path(file), std::cerr);
                int index = 0;
                for (auto outData = m_DataStore.getOutputFile(); outData.second != 0; outData = m_DataStore.getOutputFile()) {
                    outputFile(
                                (mainDir / m_OutputDir / m_BinsDir / m_TextOutDir / outData.first).string(),
                                std::vector<unsigned char>(
                                    m_DataStore.data_begin(),
                                    m_DataStore.data_end()
                                    ),
                                outData.second,
                                index
                                );
                    index += outData.second;
                }
            }
            input.close();
        }
    }
    m_DataStore.sort();

    {
        std::ofstream mainText((mainDir / m_OutputDir / "text.asm").string());
        std::ofstream textDefines((mainDir / m_OutputDir / "textDefines.exp").string());
        if (!mainText || !textDefines) {
            throw ASMError("Could not open files in " + (mainDir / m_OutputDir).string() + " for writing.\n");
        }
        RomPatcher r("lorom");
        r.writeParsedData(m_DataStore, fs::path(m_BinsDir) / m_TextOutDir, mainText, textDefines);
        textDefines.flush();
        textDefines.close();
        mainText.flush();
        mainText.close();
        for (Rom& romData: m_Roms) {
            std::ofstream mainFile((fs::path(m_MainDir) / (romData.name + ".asm")).string());
            mainFile << "lorom\n\n";
            r.writeIncludes(romData.includes.cbegin(), romData.includes.cend(), mainFile, fs::path(m_OutputDir));
            r.writeIncludes(m_Includes.cbegin(), m_Includes.cend(), mainFile, fs::path(m_OutputDir));
            r.writeIncludes(m_Extras.cbegin(), m_Extras.cend(), mainFile, fs::path(m_OutputDir) / m_BinsDir);
            mainFile << r.generateInclude(fs::path(m_OutputDir) / m_BinsDir / m_FontDir / (m_FontDir + ".asm"), fs::path(), false) + '\n';
            mainFile << r.generateInclude(fs::path(m_OutputDir) / "textDefines.exp", fs::path(), false) + '\n' +
                        r.generateInclude(fs::path(m_OutputDir) / "text.asm", fs::path(), false) + '\n';
            mainFile.close();
        }
        fs::path fontFilePath = fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_FontDir / (m_FontDir + ".asm");
        std::ofstream output(fontFilePath.string());
        if (!output) {
            throw ASMError("Could not open " + fontFilePath.string() + "for writing.\n");
        }
        r.writeIncludes(m_FontIncludes.begin(), m_FontIncludes.end(), output);
        r.writeFontData(m_DataStore, output);
        output.close();
    }
    maxAddress = (m_DataStore.end()-1)->address;
    return true;
}

void Project::writePatchData()
{
    fs::path mainDir(m_MainDir);
    for (Rom& romData: m_Roms) {
        std::string patchFile = (mainDir / (romData.name + ".asm")).string();

        fs::path romFilePath = fs::path(m_RomsDir) / romData.file;
        std::string extension = romFilePath.extension().string();
        RomPatcher r("lorom");
        r.loadRom(romFilePath.string(), romData.name, romData.hasHeader);
        r.expand(util::LoROMToPC(getMaxAddress()));
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
        } else {
            for (auto& msg: messages) {
                std::ostringstream error;
                error << msg << '\n';
                throw ASMError(error.str());
            }
        }

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

sable::Project::operator bool() const
{
    return !m_MainDir.empty();
}

void Project::outputFile(const std::string &file, const std::vector<unsigned char>& data, size_t length, int start)
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
                    if (!outputConfig[OUTPUT_BIN][FONT_SECTION][INCLUDE_VAL].IsSequence()) {
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
        if (!configYML[CONFIG_SECTION][IN_MAP].IsDefined() || !configYML[CONFIG_SECTION][IN_MAP].IsScalar()) {
            isValid = false;
            errorString << "inMapping for config section is missing or is not a scalar.\n";
        }
        if (configYML[CONFIG_SECTION][MAP_TYPE].IsDefined() && !configYML[CONFIG_SECTION][MAP_TYPE].IsScalar()) {;
            isValid = false;
            errorString << "config > mapper must be a string.\n";
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
