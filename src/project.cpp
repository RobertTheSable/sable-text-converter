#include "project.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "wrapper/filesystem.h"
#include "util.h"
#include "rompatcher.h"

namespace sable {

Project::Project(const std::string &projectDir) : nextAddress(0)
{
    if (!fs::exists(fs::path(projectDir) / "config.yml")) {
        throw std::runtime_error((fs::path(projectDir) / "config.yml").string() + " not found.");
    }
    YAML::Node config = YAML::LoadFile((fs::path(projectDir) / "config.yml").string());
    if (validateConfig(config)) {
        m_MainDir = config["files"]["mainDir"].as<std::string>();
        m_InputDir = config["files"]["input"]["directory"].as<std::string>();
        m_OutputDir = config["files"]["output"]["directory"].as<std::string>();
        m_BinsDir = config["files"]["output"]["binaries"]["mainDir"].as<std::string>();
        m_TextOutDir = config["files"]["output"]["binaries"]["textDir"].as<std::string>();
        m_FontConfig = config["files"]["output"]["binaries"]["fonts"]["dir"].as<std::string>();
        m_RomsDir = config["files"]["romDir"].as<std::string>();
        fs::path mainDir(projectDir);
        if (fs::path(m_MainDir).is_relative()) {
            m_MainDir = (mainDir / m_MainDir).string();
        }
        if (fs::path(m_RomsDir).is_relative()) {
            m_RomsDir = (mainDir / m_RomsDir).string();
        }
        fs::path fontLocation = mainDir / config["config"]["directory"].as<std::string>() / config["config"]["inMapping"].as<std::string>();
        m_Parser = TextParser(YAML::LoadFile(fontLocation.string()), config["config"]["defaultMode"].as<std::string>());
        if (config["files"]["output"]["binaries"]["fonts"]["includes"].IsSequence()) {
            m_FontIncludes = config["files"]["output"]["binaries"]["fonts"]["includes"].as<std::vector<std::string>>();
        }
        if (config["files"]["output"]["binaries"]["extras"].IsSequence()) {
            m_Extras = config["files"]["output"]["binaries"]["extras"].as<std::vector<std::string>>();
        }
        if (config["files"]["output"]["includes"].IsSequence()){
            m_Includes = config["files"]["output"]["includes"].as<std::vector<std::string>>();
        }
        m_Roms = config["roms"].as<std::vector<Rom>>();
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
                    fs::path dir(fs::path(path).parent_path());
                    std::string label = dir.filename().string();
                    Table table;
                    table.setAddress(nextAddress);
                    try {
                        files = table.getDataFromFile(tablefile);
                        for (std::string& file: files) {
                            if (!fs::exists(dir / file)) {
                                std::cerr << "In " + path
                                             + ": file " + fs::absolute(dir / file).string()
                                             + " does not exist."
                                          << std::endl;
                            }
                            file = (dir / file).string();
                        }
                    } catch (std::runtime_error &e) {
                        throw std::runtime_error("In " + path + ", " + e.what());
                    }

                    tablefile.close();
                    m_TableList[label] = table;
                    m_Addresses.push_back({table.getAddress(), label, true});
                } else {
                    for (auto& fileIt: fs::directory_iterator(dir)) {
                        if(fs::is_regular_file(fileIt.path())) {
                            files.push_back(fileIt.path().string());
                        }
                    }
                }
                std::sort(files.begin(), files.end());
                allFiles.insert(allFiles.end(), files.begin(), files.end());
            }
        }
        std::string dir = "";
        int dirIndex;
        for (auto &file: allFiles) {
            if (dir != fs::path(file).parent_path().filename().string()) {
                dir = fs::path(file).parent_path().filename().string();
                nextAddress = m_TableList[dir].getDataAddress();
                dirIndex = 0;
            }
            std::ifstream input(file);
            std::vector<unsigned char> data;
            ParseSettings settings =  m_Parser.getDefaultSetting(nextAddress);
            int line = 0;
            while (input) {
                bool done;
                int length;
                try {
                    std::tie(done, length) = m_Parser.parseLine(input, settings, std::back_inserter(data));
                    line++;
                } catch (std::runtime_error &e) {
                    std::cerr << "In file " << file << ": " << e.what() << std::endl;
                    return false;
                }
                if (settings.maxWidth > 0 && length > settings.maxWidth) {
                    std::cerr << "In " + file + ": line " + std::to_string(line)
                                 + " is longer than the specified max width of "
                                 + std::to_string(settings.maxWidth) + " pixels."
                              << std::endl;
                }
                if (done && !data.empty()) {
                    if (m_Parser.getFonts().at(settings.mode)) {
                        if (settings.label.empty()) {
                            std::ostringstream lstream;
                            lstream << dir << '_' << dirIndex++;
                            settings.label = lstream.str();
                        }
                        m_Addresses.push_back({settings.currentAddress, settings.label, false});
                        {
                            int tmpAddress = settings.currentAddress + data.size();
                            size_t dataLength;
                            fs::path binFileName = mainDir / m_OutputDir / m_BinsDir / m_TextOutDir / (settings.label + ".bin");
                            bool printpc = settings.printpc;
                            if ((tmpAddress & 0xFF0000) != (settings.currentAddress & 0xFF0000)) {
                                size_t bankLength = ((settings.currentAddress + data.size()) & 0xFFFF);
                                dataLength = data.size() - bankLength;
                                fs::path bankFileName = binFileName.parent_path() / (settings.label + "bank.bin");
                                outputFile(bankFileName.string(), data, bankLength, dataLength);
                                settings.currentAddress = util::PCToLoROM(util::LoROMToPC(settings.currentAddress | 0xFFFF) +1);
                                m_Addresses.push_back({settings.currentAddress, "$" + settings.label, false});
                                settings.currentAddress += bankLength;
                                m_TextNodeList["$" + settings.label] = {
                                    bankFileName.filename().string(),
                                    bankLength,
                                    printpc
                                };
                                printpc = false;
                            } else {
                                dataLength = data.size();
                                settings.currentAddress += data.size();
                            }
                            outputFile(binFileName.string(), data, dataLength);
                            m_TextNodeList[settings.label] = {
                                binFileName.filename().string(), dataLength, printpc
                            };
                        }
                        settings.label = "";
                        settings.printpc = false;
                        data.clear();
                    }
                }
            }
            nextAddress = settings.currentAddress;
            if (settings.maxWidth < 0) {
                settings.maxWidth = 0;
            }
        }
    }

    {
        std::ofstream mainText((mainDir / m_OutputDir / "text.asm").string());
        std::ofstream textDefines((mainDir / m_OutputDir / "textDefines.exp").string());

        std::sort(m_Addresses.begin(), m_Addresses.end(), [](AddressNode a, AddressNode b) {
            return b.address > a.address;
        });
        //int lastPosition = 0;
        for (auto& it : m_Addresses) {
            //int dif = it.address - lastPosition;
            if (it.isTable) {
                textDefines << "!def_table_" + it.label + " = $" << std::hex << it.address << '\n';
                mainText  << "ORG !def_table_" + it.label + '\n';
                Table t = m_TableList.at(it.label);
                mainText << "table_" + it.label + ":\n";
                for (auto it : t) {
                    int size;
                    std::string dataType;
                    if (t.getAddressSize() == 3) {
                        dataType = "dl";
                    } else if(t.getAddressSize() == 2) {
                        dataType = "dw";
                    } else {
                        throw std::logic_error("Unsupported address size " + std::to_string(t.getAddressSize()));
                    }
                    if (it.address > 0) {
                        mainText << dataType + " $" << std::setw(4) << std::setfill('0') << std::hex << it.address;
                        size = it.size;
                    } else {
                        mainText << dataType + ' ' + it.label;
                        size = m_TextNodeList.at(it.label).size;
                    }
                    if (t.getStoreWidths()) {
                        mainText << ", " << std::dec << size;
                    }
                    mainText << '\n';
                }
            } else {
                if (it.label.front() == '$') {
                    mainText  << "ORG $" << std::hex << it.address << '\n';
                } else {
                    textDefines << "!def_" + it.label + " = $" << std::hex << it.address << '\n';
                    mainText  << "ORG !def_" + it.label + '\n'
                              << it.label + ":\n";

                }
                std::string token = m_TextNodeList.at(it.label).files;
                mainText << "incbin " + (fs::path(m_BinsDir) / m_TextOutDir / token).string() + '\n';
                if (m_TextNodeList.at(it.label).printpc) {
                    mainText << "print pc\n";
                }
            }
            mainText << '\n';
        }
        textDefines.flush();
        textDefines.close();
        mainText.flush();
        mainText.close();
        fs::path mainDir(m_MainDir);
        for (Rom& romData: m_Roms) {
            std::string patchFile = (mainDir / (romData.name + ".asm")).string();
            std::ofstream mainFile(patchFile);
            mainFile << "lorom\n\n";
            if (!romData.includes.empty()) {
                for (std::string& include: romData.includes) {
                    mainFile << "incsrc " + m_OutputDir + '/' + include << '\n';
                }
            }
            if (!m_Includes.empty()) {
                for (std::string& include: m_Includes) {
                    mainFile << "incsrc " + m_OutputDir + '/' + include << '\n';
                }
            }
            if (!m_Extras.empty()) {
                for (std::string& include: m_Extras) {
                    mainFile << "incsrc " + m_OutputDir + '/'
                                + m_BinsDir + '/' + include + '/' + include + ".asm"
                             << '\n';
                }
            }
            if (!m_FontConfig.empty()) {
                mainFile << "incsrc " + m_OutputDir + '/'
                            + m_BinsDir + '/' + m_FontConfig + '/' + m_FontConfig + ".asm"
                         << '\n';
            }
            mainFile <<  "incsrc " + m_OutputDir + "/textDefines.exp\n"
                        +  "incsrc " + m_OutputDir + "/text.asm\n";
            mainFile.close();
        }
        writeFontData();
    }
    return true;
}

void Project::writePatchData()
{
    fs::path mainDir(m_MainDir);
    for (Rom& romData: m_Roms) {
        std::string patchFile = (mainDir / (romData.name + ".asm")).string();

        fs::path romFilePath = fs::path(m_RomsDir) / romData.file;
        std::string extension = romFilePath.extension().string();
        RomPatcher r(
                     romFilePath.string(),
                    romData.name,
                    "lorom",
                    romData.hasHeader
                    );
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
            output.write((char*)&r.at(0), r.getRealSize());
            output.close();
        } else {
            for (auto& msg: messages) {
                std::cerr << msg << std::endl;
            }
        }

    }
}

void Project::writeFontData()
{
    fs::path fontFilePath = fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_FontConfig / (m_FontConfig + ".asm");
    std::ofstream output(fontFilePath.string());
    for (std::string& include: m_FontIncludes) {
        output << "incsrc " + include + ".asm\n";
    }
    for (auto& fontIt: m_Parser.getFonts()) {
        if (!fontIt.second.getFontWidthLocation().empty()) {
            output << "\n"
                      "ORG " + fontIt.second.getFontWidthLocation();
            std::vector<int> widths;
            widths.reserve(fontIt.second.getMaxEncodedValue());
            fontIt.second.getFontWidths(std::back_insert_iterator(widths));
            int column = 0;
            int skipCount = 0;
            for (auto it = widths.begin(); it != widths.end(); ++it) {
                int width = *it;
                if (width == 0) {
                    skipCount++;
                    column = 0;
                } else {
                    if (skipCount > 0) {
                        output << "\n"
                                  "skip " << std::dec << skipCount;
                        skipCount = 0;
                    }
                    if (column == 0) {
                        output << '\n' << "db ";
                    } else {
                        output << ", ";
                    }
                    output << "$" << std::hex << std::setw(2) << std::setfill('0') << width;
                    column++;
                    if (column ==16) {
                        column = 0;
                    }
                }
            }
            output << '\n' ;
        }
    }
    output.close();
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
    return fs::absolute(fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_FontConfig).string();
}

std::string Project::TextOutDir() const
{
    return fs::absolute(fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_TextOutDir).string();
}

int Project::getMaxAddress() const
{
    if (m_Addresses.empty()) {
        return 0;
    }
    return m_Addresses.back().address;
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
        throw std::runtime_error("Could not open " + file + " for writing");
    }
    output.write((char*)&(data[start]), length);
    output.close();
}

bool Project::validateConfig(const YAML::Node &configYML)
{
    using std::cerr;
    bool isValid = true;
    if (!configYML["files"].IsDefined() || !configYML["files"].IsMap()) {
        isValid = false;
        cerr << "Error: files section is missing or not a map.\n";
    } else {
        std::string mainDir = "";
        if (configYML["files"]["mainDir"].IsDefined() && configYML["files"]["mainDir"].IsScalar()) {
            mainDir = configYML["files"]["mainDir"].Scalar();
        }
        if (!configYML["files"]["input"].IsDefined() || !configYML["files"]["input"].IsMap()) {
            isValid = false;
            cerr << "Error: input section is missing or not a map.\n";
        } else {
            if (!configYML["files"]["input"]["directory"].IsDefined() || !configYML["files"]["input"]["directory"].IsScalar()) {
                isValid = false;
                cerr << "Error: input directory for project is missing from files config.\n";
            }
        }
        if (!configYML["files"]["output"].IsDefined() || !configYML["files"]["output"].IsMap()) {
            isValid = false;
            cerr << "Error: output section is missing or not a map.\n";
        } else {
            if (!configYML["files"]["output"]["directory"].IsDefined() || !configYML["files"]["output"]["directory"].IsScalar()) {
                isValid = false;
                cerr << "Error: output directory for project is missing from files config.\n";
            }
            if (!configYML["files"]["output"]["binaries"].IsDefined() || !configYML["files"]["output"]["binaries"].IsMap()) {
                isValid = false;
                cerr << "Error: output binaries subdirectory section is missing or not a map.\n";
            } else {
                if (!configYML["files"]["output"]["binaries"]["mainDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["mainDir"].IsScalar()) {
                    isValid = false;
                    cerr << "Error: output binaries subdirectory mainDir value is missing from files config.\n";
                }
                if (!configYML["files"]["output"]["binaries"]["textDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["textDir"].IsScalar()) {
                    isValid = false;
                    cerr << "Error: output binaries subdirectory textDir value is missing from files config.\n";
                }
                if (configYML["files"]["output"]["binaries"]["extras"].IsDefined() && !configYML["files"]["output"]["binaries"]["extras"].IsSequence()) {
                    isValid = false;
                    cerr << "Error: extras section for output binaries must be a sequence.\n";
                }
            }
            if (configYML["files"]["output"]["includes"].IsDefined() && !configYML["files"]["output"]["includes"].IsSequence()) {
                isValid = false;
                cerr << "Error: includes section for output must be a sequence.\n";
            }
        }
        if (!configYML["files"]["romDir"].IsDefined() || !configYML["files"]["romDir"].IsScalar()) {
            isValid = false;
            cerr << "Error: romDir for project is missing from files config.\n";
        }
    }
    if (!configYML["config"].IsDefined() || !configYML["config"].IsMap()) {
        isValid = false;
        cerr << "Error: config section is missing or not a map.\n";
    } else {
        if (!configYML["config"]["directory"].IsDefined() || !configYML["config"]["directory"].IsScalar()) {
            isValid = false;
            cerr << "Error: directory for config section is missing or is not a scalar.\n";
        }
        if (!configYML["config"]["inMapping"].IsDefined() || !configYML["config"]["inMapping"].IsScalar()) {
            isValid = false;
            cerr << "Error: inMapping for config section is missing or is not a scalar.\n";
        }
        if (configYML["config"]["mapper"].IsDefined() && !configYML["config"]["mapper"].IsScalar()) {;
            isValid = false;
            cerr << "Error: config > mapper must be a string.\n";
        }
    }
    if (!configYML["roms"].IsDefined()) {
        isValid = false;
        cerr << "Error: roms section is missing.\n";
    } else {
        if (!configYML["roms"].IsSequence()) {
            isValid = false;
            cerr << "Error: roms section in config must be a sequence.\n";
        } else {
            int romindex = 0;
            for(auto node: configYML["roms"]) {
                std::string romName;
                if (!node["name"].IsDefined() || !node["name"].IsScalar()) {
                    cerr << "Error: rom at index " << romindex << " is missing a name value.\n";
                    char temp[50];
                    sprintf(temp, "at index %d", romindex);
                    romName = temp;
                    isValid = false;
                } else {
                    romName = node["name"].Scalar();
                }
                if (!node["file"].IsDefined() || !node["file"].IsScalar()) {
                    cerr << "Error: rom " << romName << " is missing a file value.\n";
                    isValid = false;
                }
                if (node["header"].IsDefined() && (!node["header"].IsScalar() || (
                node["header"].Scalar() != "auto" && node["header"].Scalar() != "true" && node["header"].Scalar() != "false"))) {
                    cerr << "Error: rom " << romName << " does not have a valid header option - must be \"true\", \"false\", \"auto\", or not defined.\n";
                    isValid = false;
                }
            }
        }
    }
    return isValid;
}
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
    if (node["includes"].IsSequence()) {
        rhs.includes = node["includes"].as<std::vector<std::string>>();
    }
    return true;
}
