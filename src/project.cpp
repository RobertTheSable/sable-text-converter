#include "project.h"
#include <yaml-cpp/yaml.h>
#include "wrapper/filesystem.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include "options.h"

namespace sable {

Project::Project(const std::string &projectDir) : nextAddress(0)
{
    YAML::Node config = YAML::LoadFile((fs::path(projectDir) / "config.yml").string());
    if (validateConfig(config)) {
        m_MainDir = config["files"]["mainDir"].as<std::string>();
        m_InputDir = config["files"]["input"]["directory"].as<std::string>();
        m_OutputDir = config["files"]["output"]["directory"].as<std::string>();
        m_BinsDir = config["files"]["output"]["binaries"]["mainDir"].as<std::string>();
        m_TextOutDir = config["files"]["output"]["binaries"]["textDir"].as<std::string>();
        m_FontConfig = config["files"]["output"]["binaries"]["fonts"]["dir"].as<std::string>();
        fs::path mainDir(projectDir);
        if (fs::path(m_MainDir).is_relative()) {
            m_MainDir = (mainDir / m_MainDir).string();
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
                            if ((tmpAddress & 0xFF0000) != (settings.currentAddress & 0xFF0000)) {
                                size_t bankLength = ((settings.currentAddress + data.size()) & 0xFFFF);
                                dataLength = data.size() - bankLength;
                                fs::path bankFileName = binFileName.parent_path() / (settings.label + "bank.bin");
                                outputFile(bankFileName.string(), data, bankLength, dataLength);
                                settings.currentAddress = PCToLoROM(LoROMToPC(settings.currentAddress | 0xFFFF) +1);
                                m_Addresses.push_back({settings.currentAddress, "$" + settings.label, false});
                                settings.currentAddress += bankLength;
                                m_TextNodeList["$" + settings.label] = {
                                    bankFileName.filename().string(),
                                    bankLength
                                };
                            } else {
                                dataLength = data.size();
                                settings.currentAddress += data.size();
                            }
                            outputFile(binFileName.string(), data, dataLength);
                            m_TextNodeList[settings.label] = {
                                binFileName.filename().string(), dataLength
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
            }
            mainText << '\n';
        }
        textDefines.flush();
        textDefines.close();
        mainText.flush();
        mainText.close();
    }
    return true;
}

void Project::writeOutput()
{
    fs::path mainDir(m_MainDir);
    for (Rom& romData: m_Roms) {
        std::ofstream mainFile((mainDir / (romData.name + ".asm")).string());
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
        if (!m_FontIncludes.empty()) {
            for (std::string& include: m_FontIncludes) {
                mainFile << "incsrc " + m_OutputDir + '/'
                            + m_BinsDir + '/' + m_FontConfig + '/' + include + ".asm"
                         << '\n';
            }
        }
        mainFile <<  "incsrc " + m_OutputDir + "/textDefines.exp\n"
                    +  "incsrc " + m_OutputDir + "/text.asm\n";
        mainFile.close();
    }
}

void Project::writeFontData()
{
    for (auto include: m_FontIncludes) {

    }
}

std::string Project::MainDir() const
{
    return m_MainDir;
}

std::string Project::RomsDir() const
{
    return fs::absolute(fs::path(m_MainDir) / ".." / m_RomsDir).string();
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
