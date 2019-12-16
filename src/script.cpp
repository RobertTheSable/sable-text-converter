#include "script.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#ifdef _WIN32
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
#include "lib/utf8/utf8.h"

using std::setw;
using std::endl;
using std::hex;
#ifdef _WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

//dialogue max width: 8 * 20 px
int PCToLoROM(int addr, bool header = false)
{
    if (header) {
        addr-=512;
    }
    if (addr<0 || addr>=0x400000) {
        return -1;
    }
    addr=((addr<<1)&0x7F0000)|(addr&0x7FFF)|0x8000;
    addr|=0x800000;
    return addr;
}
int PCToLoROMLow(int addr, bool header = false)
{
    if (header) {
        addr-=512;
    }
    if (addr<0 || addr>=0x400000) {
        return -1;
    }
    addr=((addr<<1)&0x7F0000)|(addr&0x7FFF)|0x8000;
    return addr;
}
int LoROMToPC(int addr, bool header = false)
{
    if (addr<0 || addr>0xFFFFFF ||//not 24bit
                (addr&0xFE0000)==0x7E0000 ||//wram
                (addr&0x408000)==0x000000)//hardware regs
            return -1;
    addr=((addr&0x7F0000)>>1|(addr&0x7FFF));
    if (header) addr+=512;
    return addr;
}

std::string getUtf8Char(std::string::iterator& start, std::string::iterator end, bool advance = true)
{
    auto currentIt = start;
    if (advance) {
        utf8::advance(start, 1, end);
        return std::string(currentIt, start);
    } else {
        auto tempIt = start;
        utf8::advance(tempIt, 1, end);
        return std::string(currentIt, tempIt);
    }
}

Script::Script() : isScriptValid(false)
{
    fs::path configFile= "config";
    fs::path outFile = configFile / "out.yml";
    configFile /= "in.yml";
    if (fs::exists(configFile)) {
        inConfig = YAML::LoadFile(configFile.string());
    }
}

Script::Script(const char *configFile) : isScriptValid(false)
{
    fs::path configFilePath = configFile;
    if (fs::exists(configFilePath)) {
        inConfig = YAML::LoadFile(configFile);
    }
}

void Script::loadScript(const char *inDir, std::string defaultMode)
{
    fs::path input = inDir;

    for (auto& dirItr: fs::directory_iterator(inDir)) {
        std::string tableName = dirItr.path().string() + "Table";
        unsigned int nextAddress = 0;
        int tableAddresssDataWidth = 3;

        if(fs::is_directory(dirItr.path())) {
            int fileindex = 0;
            bool isTable = false;
            std::vector<fs::path> files;
            std::vector<std::string> tableEntries;
            std::map<std::string, int> binLengths;
            std::stringstream tableTextStream;
            int tableAddress = 0;
            int tableLength = 0;
            bool isFileListValid = true;
            bool storeWidths = false;
            if (fs::exists(dirItr.path() / "table.txt")) {
                isTable= true;
                std::ifstream tablefile((dirItr.path() / "table.txt").string());
                std::string input;
                while (isFileListValid && tablefile >> input) {
                    std::string option;
                    if (input == "address") {
                        tablefile >> option;
                        try {
                            std::string::size_type sz;
                            tableAddress = std::stoi(option, &sz, 16);
                            if (LoROMToPC(tableAddress) == -1 || sz != option.length()) {
                                throw std::invalid_argument("");
                            }
                            labels[tableAddress] = std::string("table_") + dirItr.path().filename().string();
                            tableTextStream << labels[tableAddress] << ":" << endl;
                        } catch (const std::invalid_argument& e) {
                            std::cerr << "Error in table file: " << option << " is not a valid SNES address." << endl;
                            isFileListValid = false;
                        }
                    } else if (input == "file") {
                        if (tablefile >> option) {
                            files.push_back(dirItr.path() /option);
                        }
                    } else if (input == "entry") {
                        if (tablefile >> option) {
                            if (option == "const") {
                                getline(tablefile, option, '\n');
                                if(option.find('\r') != std::string::npos) {
                                   option.erase(option.find('\r'), 1);
                                }
                                tableEntries.push_back(option);
                            } else {
                                tableEntries.push_back( option);
                            }
                            tableLength += tableAddresssDataWidth;
                            if (storeWidths) {
                                tableLength += tableAddresssDataWidth;
                            }
                        }
                    } else if (input == "data") {
                        if (tablefile >> option) {
                            try {
                                std::string::size_type sz;
                                nextAddress = std::stoi(option, &sz, 16);
                                if (LoROMToPC(nextAddress) == -1|| sz != option.length()) {
                                    throw std::invalid_argument("");
                                }
                            } catch (const std::invalid_argument& e) {
                                std::cerr << "Error in table file: " << option << " is not a valid SNES address." << endl;
                                isFileListValid = false;
                            }

                        }
                    } else if (input == "width") {
                        if (tablefile >> option) {
                            try {
                                tableAddresssDataWidth = std::stoi(option);
                                if (tableAddresssDataWidth != 2 && tableAddresssDataWidth != 3) {
                                    std::cerr << "Error in table file: width value should be 2 or 3." << endl;
                                    isFileListValid = false;
                                }
                            } catch (const std::invalid_argument& e) {
                                std::cerr << "Error in table file: width value should be 2 or 3." << endl;
                                isFileListValid = false;
                            }

                        }
                    } else if (input == "savewidth") {
                        storeWidths = true;
                    }
                }
                if (nextAddress == 0) {
                    nextAddress = tableAddress + tableLength;
                }
                tablefile.close();
            } else {
                for (auto& fileIt: fs::directory_iterator(dirItr.path())) {
                    if(fs::is_regular_file(fileIt.path())) {
                        files.push_back(fileIt.path());
                    }
                }
            }
            if (isFileListValid) {
                std::sort(files.begin(), files.end());
                std::string textType = defaultMode;
                bool digraphs = inConfig["TextTypes"][textType]["HasDigraphs"].Scalar() == "true";
                int byteWidth = inConfig["TextTypes"][textType]["ByteWidth"].as<int>();
                int commandValue = inConfig["TextTypes"][textType]["CommandValue"].IsDefined() ? inConfig["TextTypes"][textType]["CommandValue"].as<int>() : -1;
                int fixedWidth = inConfig["TextTypes"][textType]["FixedWidth"].IsDefined() ? inConfig["TextTypes"][textType]["FixedWidth"].as<int>() : -1;
                int maxWidth = inConfig["TextTypes"][textType]["MaxWidth"].IsDefined() ? inConfig["TextTypes"][textType]["MaxWidth"].as<int>() : 0;
                for (auto textFileItr: files) {
                    textNode currentTextNode = {nextAddress, "", commandValue == -1, storeWidths, false, maxWidth, {}};
                    std::ifstream textFile(textFileItr.string());
                    std::string input;
                    int lineNumber = 1;
                    bool fileMaxWidth = false;
                    bool isFileValid = true;
                    bool autoEnd = true;
                    while (isFileValid && getline(textFile, input, '\n')) {
                        if(input.find('\r') != std::string::npos) {
                           input.erase(input.find('\r'), 1);
                        }
                        int currentLineLength = 0;
                        auto stringChar = input.begin();
                        bool parsing = true;
                        std::vector<unsigned short> line = {};
                        bool parseNewLines = true;
                        if (input.empty()) {
                            if (textFile.peek() != '#' && textFile.peek() != '@') {
                                if (commandValue == -1) {
                                    currentTextNode.data.push_back(
                                                inConfig["TextTypes"][textType]["Commands"]["NewLine"]["code"].as<int>()
                                            );
                                } else {
                                    currentTextNode.data.push_back(commandValue);
                                    currentTextNode.data.push_back(
                                        inConfig["TextTypes"][textType]["Commands"]["NewLine"]["code"].as<int>()
                                            );
                                }
                            }
                        } else {
                            while (isFileValid && parsing) {
                                std::string current = getUtf8Char(stringChar, input.end());
                                if (current == "#") {
                                    parsing = false;
                                } else if (current == "@") {
                                    parseNewLines = false;
                                    std::stringstream configString(input.substr(stringChar - input.begin()));
                                    std::string settingName, settingOption;
                                    if (configString >> settingName) {
                                        if (configString >> settingOption) {
                                            if (settingName == "address") {
                                                if (settingOption == "auto") {
                                                    if (currentTextNode.address == 0) {
                                                        // no address is set yet, error
                                                        std::cerr << "Warning: On line " << lineNumber << ", attempted to determine automatic address when one was not set before." << endl;
                                                    }
                                                } else {
                                                    if (settingOption[0] == '$') {
                                                        settingOption = settingOption.substr(1);
                                                    }
                                                    try {
                                                        unsigned int data = std::stoi(settingOption, nullptr, 16);
                                                        if (LoROMToPC(data) == -1) {
                                                            std::cerr << "Error: " << settingOption << " is not a valid SNES address." << endl;
                                                            isFileValid = false;
                                                        }
                                                        currentTextNode.address = data;
                                                    } catch (const std::invalid_argument& e) {
                                                        std::cerr << "Error: " << settingOption << " is not a hexadecimal address." << endl;
                                                        isFileValid = false;
                                                    }
                                                }
                                            } else if (settingName == "type") {
                                                if (settingOption == "default") {
                                                    settingOption = defaultMode;
                                                }
                                                if (inConfig["TextTypes"][settingOption].IsDefined() && inConfig["TextTypes"][settingOption].IsMap()) {
                                                    textType = settingOption;
                                                    digraphs = inConfig["TextTypes"][textType]["HasDigraphs"].Scalar() == "true";
                                                    byteWidth = inConfig["TextTypes"][textType]["ByteWidth"].as<int>();
                                                    commandValue = inConfig["TextTypes"][textType]["CommandValue"].IsDefined() ? inConfig["TextTypes"][textType]["CommandValue"].as<int>() : -1;
                                                    fixedWidth = inConfig["TextTypes"][textType]["FixedWidth"].IsDefined() ? inConfig["TextTypes"][textType]["FixedWidth"].as<int>() : -1;
                                                    if (!fileMaxWidth) {
                                                        maxWidth = inConfig["TextTypes"][textType]["MaxWidth"].IsDefined() ? inConfig["TextTypes"][textType]["MaxWidth"].as<int>() : 0;
                                                        currentTextNode.maxWidth = maxWidth;
                                                    }
                                                    currentTextNode.isMenuText = (commandValue == -1);
                                                } else {
                                                    throw std::runtime_error(std::string("Error: type ") + settingOption + " is not defined in config file.");
                                                }
                                            } else if (settingName == "width") {
                                                if (settingOption == "off") {
                                                    currentTextNode.maxWidth = 0;
                                                    fileMaxWidth = true;
                                                } else  {
                                                    try {
                                                        currentTextNode.maxWidth = std::stoi(settingOption);
                                                        fileMaxWidth = true;
                                                    } catch (const std::invalid_argument& e) {
                                                        std::cerr << "Warning: width " << settingOption << " on line " << lineNumber << " is not valid and will be ignored - width argument needs to be \"off\" or a decimal number." << endl;
                                                    }
                                                }
                                            } else if (settingName == "label") {
                                                currentTextNode.label = settingOption;
                                            } else if (settingName == "autoend") {
                                                if (settingOption == "off") {
                                                    autoEnd = false;
                                                } else if (settingOption == "on") {
                                                    autoEnd = true;
                                                }
                                            }
                                        } else if (settingName == "printpc") {
                                            currentTextNode.printpc = true;
                                        } else {
                                            std::cerr << "Warning in " << textFileItr << ", line " << std::dec << lineNumber << ": ignorig invalid setting " << settingName << endl;
                                        }
                                    }
                                    parsing = false;
                                } else {
                                    if (currentTextNode.address == 0) {
                                        isFileValid = false;
                                        std::cerr << "Error: addess was not defined before parsing began." << endl;
                                    } else {
                                        if (current == "[") {
                                            std::string::size_type closeBracket = input.find_first_of(']', stringChar - input.begin());
                                            if (closeBracket != std::string::npos) {
                                                int convertedValue = -1;
                                                std::string convert = input.substr(stringChar - input.begin(), closeBracket-(stringChar - input.begin()));
                                                if (isFileValid) {
                                                    stringChar += convert.length()+1;
                                                }
                                                int dataWidth = 0;
                                                int formatWidth = currentTextNode.isMenuText ? 2 : 1;
                                                std::string config = "";
                                                try {
                                                    std::string::size_type sz;
                                                    int data = std::stoi(convert, &sz, 16);
                                                    if (sz != convert.length()) {
                                                        throw std::invalid_argument("");
                                                    }
                                                    dataWidth = (convert.length()+1)>>byteWidth;
                                                    if (dataWidth > 3) {
                                                        std::cerr << "Error in " << textFileItr << " on line " << lineNumber << ": Numbers must be no more than three bytes long.";
                                                        isFileListValid = false;
                                                    } else {
                                                        convertedValue = data;
                                                    }
                                                } catch (const std::invalid_argument& e) {
                                                    dataWidth = byteWidth;
                                                    if (inConfig["TextTypes"][textType]["Commands"][convert].IsDefined()) {
                                                        // normal text command
                                                        if (commandValue != -1) {
                                                            line.push_back(commandValue);
                                                        }
                                                        config = "Commands";
                                                        convertedValue = inConfig["TextTypes"][textType]["Commands"][convert]["code"].as<int>();
                                                        parseNewLines = !inConfig["TextTypes"][textType]["Commands"][convert]["newline"].IsDefined();
                                                    } else {
                                                        std::string withBrackets  = std::string({'['}) + convert + ']';
                                                        if (inConfig["TextTypes"][textType]["Encoding"][withBrackets].IsDefined()) {
                                                            config = "Encoding";
                                                            convert = withBrackets;
                                                            convertedValue = inConfig["TextTypes"][textType]["Encoding"][convert]["code"].as<int>();
                                                            if(currentTextNode.maxWidth > 0 && inConfig["TextTypes"][textType]["Encoding"][convert]["length"].IsDefined()) {
                                                                if (fixedWidth != -1) {
                                                                    currentLineLength += fixedWidth;
                                                                } else {
                                                                    currentLineLength += inConfig["TextTypes"][textType]["Encoding"][convert]["length"].as<int>();
                                                                }
                                                            }
                                                        } else if (inConfig["TextTypes"][textType]["Extras"][convert].IsDefined() && inConfig["TextTypes"][textType]["Extras"][convert].IsScalar()) {
                                                            config = "Extras";
                                                            convertedValue = inConfig["TextTypes"][textType]["Extras"][convert].as<int>();
                                                        } else {
                                                            std::cerr << "Error in " << textFileItr << ": " << convert << " is not valid hexadecimal or is not a valid command/extra define." << endl;
                                                            isFileValid = false;
                                                        }
                                                    }
                                                }
                                                if (convertedValue >= 0) {
                                                    do {
                                                        unsigned short pushValue = convertedValue & (currentTextNode.isMenuText ? 0xFFFF: 0xFF);
                                                        line.push_back(pushValue);
                                                        if (!config.empty()) {
                                                            if (autoEnd && (
                                                                    (inConfig["TextTypes"][textType][config]["[End]"].IsDefined() && pushValue == inConfig["TextTypes"][textType][config]["[End]"]["code"].as<int>()) ||
                                                                    (inConfig["TextTypes"][textType][config]["End"].IsDefined() && pushValue == inConfig["TextTypes"][textType][config]["End"]["code"].as<int>()))) {
                                                                if (!line.empty()) {
                                                                    currentTextNode.data.insert(currentTextNode.data.end(), line.begin(), line.end());
                                                                    line = {};
                                                                    parseNewLines = false;
                                                                }
                                                                if(currentTextNode.label.empty()) {
                                                                    currentTextNode.label = dirItr.path().filename().string() + std::to_string(fileindex++);
                                                                }
                                                                binNodes.push_back(currentTextNode);
                                                                labels[currentTextNode.address] = currentTextNode.label;
                                                                if (isTable) {
                                                                    binLengths[currentTextNode.label] = currentTextNode.data.size() * (currentTextNode.isMenuText ? 2 : 1);
                                                                }
                                                                nextAddress = currentTextNode.address + ((currentTextNode.isMenuText) ? currentTextNode.data.size()*2 : currentTextNode.data.size());
                                                                currentTextNode = {nextAddress, "", currentTextNode.isMenuText, currentTextNode.storeWidth, false, currentTextNode.maxWidth, {}};
                                                            }
                                                        }
                                                        convertedValue >>= (8*formatWidth);
                                                        dataWidth-= formatWidth;
                                                    } while (dataWidth >= formatWidth);
                                                }
                                            }
                                        } else {
                                            std::string encodingString = std::string({current});
                                            if (inConfig["TextTypes"][textType]["Encoding"][encodingString].IsDefined()) {
                                                unsigned short value;
                                                if (digraphs &&
                                                        stringChar != input.end() &&
                                                        inConfig["TextTypes"][textType]["Encoding"][encodingString + getUtf8Char(stringChar, input.end(), false)].IsDefined()
                                                    ) {
                                                    encodingString = encodingString + getUtf8Char(stringChar, input.end());
                                                }
                                                value = inConfig["TextTypes"][textType]["Encoding"][encodingString]["code"].as<int>();
                                                if(currentTextNode.maxWidth > 0) {
                                                    if (fixedWidth != -1 && inConfig["TextTypes"][textType]["Encoding"][encodingString]["length"].IsDefined()) {
                                                        currentLineLength += fixedWidth;
                                                    } else {
                                                        currentLineLength += inConfig["TextTypes"][textType]["Encoding"][encodingString]["length"].as<int>();
                                                    }
                                                }
                                                line.push_back(value);
                                            } else {
                                                isFileValid = false;
                                            }
                                        }
                                    }
                                }
                                if(stringChar == input.end()) {
                                    parsing = false;
                                }
                            }
                        }
                        if (currentTextNode.maxWidth > 0) {
                            if (currentLineLength > currentTextNode.maxWidth) {
                                std::cerr << "Warning: line " << lineNumber << " in file " << textFileItr << " is longer than the specified max width of " << currentTextNode.maxWidth << " pixels." << endl;
                            }
                        }
                        lineNumber++;
                        if (!line.empty()) {
                            currentTextNode.data.insert(currentTextNode.data.end(), line.begin(), line.end());
                            bool addNewline  = false;
                            if (textFile.peek() != EOF && textFile.peek() != '@' && textFile.peek() != '#' && parseNewLines) {
                                addNewline = true;
                            } /*else if (textFile.peek() == EOF) {
                                textFile.clear();
                                textFile.unget();
                                char test = textFile.get();
                                if (test == '\n') {
                                    addNewline = true;
                                }
                            }*/
                            if (addNewline) {
                                if (commandValue == -1) {
                                    currentTextNode.data.push_back(
                                                inConfig["TextTypes"][textType]["Commands"]["NewLine"]["code"].as<int>()
                                            );
                                } else {
                                    currentTextNode.data.push_back(commandValue);
                                    currentTextNode.data.push_back(
                                                inConfig["TextTypes"][textType]["Commands"]["NewLine"]["code"].as<int>()
                                            );
                                }
                            }
                        }
                    }
                    if (autoEnd) {
                        if (commandValue == -1) {
                            currentTextNode.data.push_back(
                                        inConfig["TextTypes"][textType]["Commands"]["End"]["code"].as<int>()
                                    );
                        } else {
                            currentTextNode.data.push_back(commandValue);
                            currentTextNode.data.push_back(
                                        inConfig["TextTypes"][textType]["Commands"]["End"]["code"].as<int>()
                                    );
                        }
                    }
                    if (isFileValid) {
                        if(currentTextNode.label.empty()) {
                            currentTextNode.label = dirItr.path().filename().string() + std::to_string(fileindex++);
                        }
                        binNodes.push_back(currentTextNode);
                        labels[currentTextNode.address] = currentTextNode.label;
                        if (isTable) {
                            binLengths[currentTextNode.label] = currentTextNode.data.size() * byteWidth;
                        }
                        nextAddress = currentTextNode.address + (currentTextNode.data.size()* byteWidth);
                    }
                    textFile.close();
                    fileindex++;
                    digraphs = inConfig["TextTypes"][textType]["HasDigraphs"].Scalar() == "true";
                } // end file list loop
                if (isTable) {
                    for (auto it = tableEntries.begin(); it != tableEntries.end(); ++it) {
                        //
                        if(it->front() == ' ') {
                            if (tableAddresssDataWidth == 3) {
                                tableTextStream << "dl" << *it << endl;
                            } else if (tableAddresssDataWidth == 2) {
                                tableTextStream << "dw" << *it << endl;
                            }
                        } else {
                            if (binLengths.count(*it) > 0 || it->front() == '$' ){
                                if (tableAddresssDataWidth == 3) {
                                    tableTextStream << "dl " << *it;
                                } else if (tableAddresssDataWidth == 2) {
                                    tableTextStream << "dw " << *it;
                                }
                                if (storeWidths) {
                                    tableTextStream << ", $" << hex << setw(4) << std::setfill('0') << binLengths[*it];
                                }
                                tableTextStream  << endl;
                            }
                        }
                    }
                    tableText[tableAddress] = {true, tableLength, tableTextStream.str()};
                }
            } // end if file list is valid
            if (isFileListValid) {
                isScriptValid = true;
            } else {
                isScriptValid = false;
            }
        } // end if path is directory
    } //end directory for loop

}

void Script::writeScript(YAML::Node outputConfig)
{
    if (!fs::exists(outputConfig["directory"].Scalar())) {
        fs::create_directory(outputConfig["directory"].Scalar());
    }
    std::string outDir = (fs::path(outputConfig["directory"].Scalar()) / outputConfig["binaries"]["mainDir"].Scalar()).string();
    if (!fs::exists(outDir)) {
        fs::create_directory(outDir);
    }
    std::string textDir = (fs::path(outDir) / outputConfig["binaries"]["textDir"].Scalar()).string();
    if (!fs::exists(textDir)) {
        fs::create_directory(textDir);
    }
    if(isScriptValid) {
        for (auto binIt = binNodes.begin(); binIt != binNodes.end(); ++binIt) {
            int dataSize = binIt->isMenuText ? binIt->data.size()*2 : binIt->data.size();
            std::stringstream includeText;
            fs::path binFileName = textDir;
            binFileName /= binIt->label + ".bin";
            includeText << binIt->label << ":" << endl
                        << "incbin " << fs::path(textDir).parent_path().filename() / outputConfig["binaries"]["textDir"].Scalar() / binFileName.filename() << endl;
            if (binIt->printpc) {
                includeText << "print pc" << endl;
            }
            tableText[binIt->address] = {false,dataSize,includeText.str()};
            std::ofstream binFile(binFileName.string(), std::ios::binary);
            int addrCounter = binIt->address & 0xFFFF;
            for (auto dataIt = binIt->data.begin(); dataIt != binIt->data.end(); ++dataIt) {
                if (addrCounter >= 0x10000) {
                    includeText.str("");
                    binFile.close();
                    binFile.clear();
                    int newAddress = (binIt->address&0xFF0000) + 0x8000 + addrCounter;
                    int cutoffLength = (binIt->address&0xFF0000) + addrCounter - binIt->address;
                    tableText[binIt->address].length = cutoffLength;
                    binFileName = binFileName.parent_path() / (binIt->label  + "bank.bin");
                    int extrasize = dataSize - (newAddress - binIt->address);
                    includeText << "incbin " << fs::path(textDir).parent_path().filename() / outputConfig["binaries"]["textDir"].Scalar() / binFileName.filename() << endl;
                    if (binIt->printpc) {
                        includeText << "print pc" << endl;
                    }
                    tableText[newAddress] = {false,extrasize,includeText.str()};
                    binFile.open(binFileName.string(), std::ios::binary);
                    addrCounter = newAddress & 0xFFFF;
                }
                if (binIt->isMenuText) {
                    binFile.write(reinterpret_cast<const char*>(&(*dataIt)), 2);
                    addrCounter+=2;
                } else {
                    binFile.write(reinterpret_cast<const char*>(&(*dataIt)), 1);
                    addrCounter++;
                }
            }
            binFile.close();
        }
        int lastAddress = 0;
        int firstBankLabel = 0;
        std::ofstream textDefines((fs::path(outDir).parent_path() / "textDefines.exp").string());
        std::ofstream asmFile((fs::path(outDir).parent_path() / "text.asm").string());
        for (auto textIt = tableText.begin(); textIt != tableText.end(); ++textIt) {
            if (textIt->first != lastAddress) {
                int difference = textIt->first - lastAddress;
                if (difference >= 0x10) {
                    if(labels.count(textIt->first) > 0) {
                        asmFile << endl << "org !def_" << labels[textIt->first] << endl;
                        if (firstBankLabel != 0 && (firstBankLabel&0xFF0000) == (textIt->first&0xFF0000)) {
                            textDefines << "!def_" << labels[textIt->first] << " = !def_" << labels[firstBankLabel] << "+$" << hex << textIt->first-firstBankLabel << endl;
                        } else {
                            textDefines << "!def_" << labels[textIt->first] << " = $" << hex << textIt->first << endl;
                            firstBankLabel = textIt->first;
                        }
                    } else {
                        asmFile << endl << "org $" << hex << textIt->first << endl;
                    }
                } else {
                    asmFile << endl << "skip " << std::dec << difference << endl;
                }
            }
            asmFile << textIt->second.text;
            lastAddress = textIt->first + textIt->second.length;
        }
        asmFile.close();
        textDefines.close();
    }
}

Script::operator bool() const
{
    return inConfig["Normal"].IsDefined() && inConfig["Menu"].IsDefined() && inConfig["Credits"].IsDefined() && inConfig["Commands"].IsDefined() && isScriptValid;
}
