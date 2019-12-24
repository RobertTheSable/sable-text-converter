#include "script.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <memory>
#include "utf8.h"

using std::setw;
using std::endl;
using std::hex;

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
void Script::writeParseWarning(const fs::path &file, std::string message, int line)
{
    if (line == 0) {
        std::cerr << std::string("Warning in ") + file.string() + ": " + message << endl;
    } else {
        std::cerr << std::string("Warning in ") + file.string() +
                    " on line " + std::to_string(line) +": " + message << endl;
    }
}

void Script::throwParseError(fs::path&& file, std::string message, int line)
{
    if (line == 0) {
        throw std::runtime_error(
                    std::string("Error in ") + file.string() + ": " + message
                    );
    } else {
        throw std::runtime_error(
                    std::string("Error in ") + file.string() +
                    " on line " + std::to_string(line) +": " + message
                    );
    }
}

void Script::loadScript(const char *inDir, const std::string& defaultTextMode, int verbosity)
{
    defaultMode = defaultTextMode;
    if (!inConfig[defaultMode].IsDefined() ) {
        std::cerr << "Error: default text type " << defaultMode << " is not defined in inMapping config file, aborting." << std::endl;
        isScriptValid = false;
        return;
    }
    try {
        for (auto& dirItr: fs::directory_iterator(fs::path(inDir))) {
            std::string tableName = dirItr.path().string() + "Table";
            nextAddress = 0;
            int tableAddresssDataWidth = 3;

            if(fs::is_directory(dirItr.path())) {
                fileindex = 0;
                bool isTable = false;
                std::vector<fs::path> files;
                std::vector<std::string> tableEntries;
                std::stringstream tableTextStream;
                int tableAddress = 0;
                int tableLength = 0;
                bool isFileListValid = true;
                bool storeWidths = false;
                fs::path tablePath(dirItr.path() / "table.txt");
                if (fs::exists(tablePath)) {
                    isTable= true;
                    std::ifstream tablefile(tablePath.string());
                    if (verbosity > 1) {
                        std::cout << "Now reading: "
                                << dirItr.path().string() + fs::path::preferred_separator + "table.txt" << endl;
                    }
                    std::string input;
                    int tableLine = 1;
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
                                labels[tableAddress] = "table_" + dirItr.path().filename().string();
                                tableTextStream << labels[tableAddress] << ":" << endl;
                            } catch (const std::invalid_argument& e) {
                                throwParseError(absolute(tablePath), option + " is not a valid SNES address.", tableLine);
                            }
                        } else if (input == "file") {
                            if (tablefile >> option) {
                                if (fs::exists(dirItr.path() / option)) {
                                    files.push_back(dirItr.path() /option);
                                } else {
                                    writeParseWarning(fs::absolute(tablePath), fs::absolute(dirItr.path() / option).string() + " does not exist.", tableLine);
                                }
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
                                    tableEntries.push_back(option);
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
                                    throwParseError(absolute(tablePath), option + " is not a valid SNES address.", tableLine);
                                }

                            }
                        } else if (input == "width") {
                            if (tablefile >> option) {
                                try {
                                    tableAddresssDataWidth = std::stoi(option);
                                    if (tableAddresssDataWidth != 2 && tableAddresssDataWidth != 3) {
                                        throwParseError(absolute(tablePath), "width value should be 2 or 3.", tableLine);
                                    }
                                } catch (const std::invalid_argument& e) {
                                    throwParseError(absolute(tablePath), "width value should be 2 or 3.", tableLine);
                                }

                            }
                        } else if (input == "savewidth") {
                            storeWidths = true;
                        }
                        tableLine++;
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
                    digraphs = inConfig[textType][USE_DIGRAPHS].Scalar() == "true";
                    byteWidth = inConfig[textType][BYTE_WIDTH].as<int>();
                    commandValue = inConfig[textType][CMD_CHAR].IsDefined() ? inConfig[textType][CMD_CHAR].as<int>() : -1;
                    fixedWidth = inConfig[textType][FIXED_WIDTH].IsDefined() ? inConfig[textType][FIXED_WIDTH].as<int>() : -1;
                    maxWidth = inConfig[textType][MAX_WIDTH].IsDefined() ? inConfig[textType][MAX_WIDTH].as<int>() : 0;
                    for (auto textFileItr: files) {
                        if (verbosity > 1) {
                            std::cout << "Now reading: "
                                    << fs::absolute(textFileItr).string() << endl;
                        }
                        parseScriptFile(textFileItr, textType, isTable, storeWidths);
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
            } // end if path is directory
        } //end directory for loop
        isScriptValid = true;
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << endl;
        isScriptValid = false;
    }
}

void Script::parseScriptFile(const fs::path &file, std::string& textType, bool isTable, bool storeWidths)
{
    textNode currentTextNode = {nextAddress, "", commandValue == -1, storeWidths, false, maxWidth, {}};
    std::ifstream textFile(file.string());
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
                                inConfig[textType]["Commands"]["NewLine"]["code"].as<int>()
                            );
                } else {
                    currentTextNode.data.push_back(commandValue);
                    currentTextNode.data.push_back(
                        inConfig[textType]["Commands"]["NewLine"]["code"].as<int>()
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
                                        writeParseWarning(fs::absolute(file), "attempted to determine automatic address when one was not set before.", lineNumber);
                                    }
                                } else {
                                    if (settingOption[0] == '$') {
                                        settingOption = settingOption.substr(1);
                                    }
                                    try {
                                        unsigned int data = std::stoi(settingOption, nullptr, 16);
                                        if (LoROMToPC(data) == -1) {
                                            throwParseError(fs::absolute(file), settingOption + " is not a hexadecimal address.", lineNumber);
                                        }
                                        currentTextNode.address = data;
                                    } catch (const std::invalid_argument& e) {
                                        throwParseError(fs::absolute(file), settingOption + " is not a hexadecimal address.", lineNumber);
                                    }
                                }
                            } else if (settingName == "type") {
                                if (settingOption == "default") {
                                    settingOption = defaultMode;
                                }
                                if (inConfig[settingOption].IsDefined() && inConfig[settingOption].IsMap()) {
                                    textType = settingOption;
                                    digraphs = inConfig[textType][USE_DIGRAPHS].Scalar() == "true";
                                    byteWidth = inConfig[textType][BYTE_WIDTH].as<int>();
                                    commandValue = inConfig[textType][CMD_CHAR].IsDefined() ? inConfig[textType][CMD_CHAR].as<int>() : -1;
                                    fixedWidth = inConfig[textType][FIXED_WIDTH].IsDefined() ? inConfig[textType][FIXED_WIDTH].as<int>() : -1;
                                    if (!fileMaxWidth) {
                                        maxWidth = inConfig[textType][MAX_WIDTH].IsDefined() ? inConfig[textType][MAX_WIDTH].as<int>() : 0;
                                        currentTextNode.maxWidth = maxWidth;
                                    }
                                    currentTextNode.isMenuText = (commandValue == -1);
                                } else {
                                    throwParseError(fs::absolute(file), std::string("type ") + settingOption + " is not defined in config file.", lineNumber);
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
                                        writeParseWarning(
                                                    fs::absolute(file),
                                                    "Ignorig invalid setting " + settingOption + " - width argument needs to be \"off\" or a decimal number.",
                                                    lineNumber
                                                    );
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
                            writeParseWarning(fs::absolute(file), "Ignorig invalid setting " + settingName, lineNumber);
                        }
                    }
                    parsing = false;
                } else {
                    if (currentTextNode.address == 0) {
                        throwParseError(absolute(file), "Address was not defined before parsing began.", lineNumber);
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
                                        throwParseError(fs::absolute(file), "Numbers must be no more than three bytes long.", lineNumber);
                                    } else {
                                        convertedValue = data;
                                    }
                                } catch (const std::invalid_argument& e) {
                                    dataWidth = byteWidth;
                                    if (inConfig[textType]["Commands"][convert].IsDefined()) {
                                        // normal text command
                                        if (commandValue != -1) {
                                            line.push_back(commandValue);
                                        }
                                        config = "Commands";
                                        convertedValue = inConfig[textType]["Commands"][convert]["code"].as<int>();
                                        parseNewLines = !inConfig[textType]["Commands"][convert]["newline"].IsDefined();
                                    } else {
                                        std::string withBrackets  = std::string({'['}) + convert + ']';
                                        if (inConfig[textType]["Encoding"][withBrackets].IsDefined()) {
                                            config = "Encoding";
                                            convert = withBrackets;
                                            convertedValue = inConfig[textType]["Encoding"][convert]["code"].as<int>();
                                            if(currentTextNode.maxWidth > 0 && inConfig[textType]["Encoding"][convert]["length"].IsDefined()) {
                                                if (fixedWidth != -1) {
                                                    currentLineLength += fixedWidth;
                                                } else {
                                                    currentLineLength += inConfig[textType]["Encoding"][convert]["length"].as<int>();
                                                }
                                            }
                                        } else if (inConfig[textType]["Extras"][convert].IsDefined() && inConfig[textType]["Extras"][convert].IsScalar()) {
                                            config = "Extras";
                                            convertedValue = inConfig[textType]["Extras"][convert].as<int>();
                                        } else {
                                            throwParseError(fs::absolute(file), convert + " is not valid hexadecimal or is not a valid command/extra define.", lineNumber);
                                        }
                                    }
                                }
                                if (convertedValue >= 0) {
                                    do {
                                        unsigned short pushValue = convertedValue & (currentTextNode.isMenuText ? 0xFFFF: 0xFF);
                                        line.push_back(pushValue);
                                        if (!config.empty()) {
                                            if (autoEnd && (
                                                    (inConfig[textType][config]["[End]"].IsDefined() && pushValue == inConfig[textType][config]["[End]"]["code"].as<int>()) ||
                                                    (inConfig[textType][config]["End"].IsDefined() && pushValue == inConfig[textType][config]["End"]["code"].as<int>()))) {
                                                if (!line.empty()) {
                                                    currentTextNode.data.insert(currentTextNode.data.end(), line.begin(), line.end());
                                                    line = {};
                                                    parseNewLines = false;
                                                }
                                                if(currentTextNode.label.empty()) {
                                                    currentTextNode.label = file.parent_path().filename().string() + std::to_string(fileindex++);
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
                            if (inConfig[textType]["Encoding"][encodingString].IsDefined()) {
                                unsigned short value;
                                if (digraphs &&
                                        stringChar != input.end() &&
                                        inConfig[textType]["Encoding"][encodingString + getUtf8Char(stringChar, input.end(), false)].IsDefined()
                                    ) {
                                    encodingString = encodingString + getUtf8Char(stringChar, input.end());
                                }
                                value = inConfig[textType]["Encoding"][encodingString]["code"].as<int>();
                                if(currentTextNode.maxWidth > 0) {
                                    if (fixedWidth != -1 && inConfig[textType]["Encoding"][encodingString]["length"].IsDefined()) {
                                        currentLineLength += fixedWidth;
                                    } else {
                                        currentLineLength += inConfig[textType]["Encoding"][encodingString]["length"].as<int>();
                                    }
                                }
                                line.push_back(value);
                            } else {
                                throwParseError(fs::absolute(file), std::string("Character '") + encodingString + "' is not defined for TextType " + textType, lineNumber);
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
                writeParseWarning(
                    fs::absolute(file),
                    "Line is longer than the specified max width of " + std::to_string(currentTextNode.maxWidth) + " pixels.",
                    lineNumber
                    );
            }
        }
        lineNumber++;
        if (!line.empty()) {
            currentTextNode.data.insert(currentTextNode.data.end(), line.begin(), line.end());
            bool addNewline  = false;
            if (textFile.peek() != EOF && textFile.peek() != '@' && textFile.peek() != '#' && parseNewLines) {
                addNewline = true;
            }
            if (addNewline) {
                if (commandValue == -1) {
                    currentTextNode.data.push_back(
                                inConfig[textType]["Commands"]["NewLine"]["code"].as<int>()
                            );
                } else {
                    currentTextNode.data.push_back(commandValue);
                    currentTextNode.data.push_back(
                                inConfig[textType]["Commands"]["NewLine"]["code"].as<int>()
                            );
                }
            }
        }
    }
    if (autoEnd) {
        if (commandValue == -1) {
            currentTextNode.data.push_back(
                        inConfig[textType]["Commands"]["End"]["code"].as<int>()
                    );
        } else {
            currentTextNode.data.push_back(commandValue);
            currentTextNode.data.push_back(
                        inConfig[textType]["Commands"]["End"]["code"].as<int>()
                    );
        }
    }
    if (isFileValid) {
        if(currentTextNode.label.empty()) {
            currentTextNode.label = file.parent_path().filename().string() + std::to_string(fileindex++);
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
    digraphs = inConfig[textType][USE_DIGRAPHS].Scalar() == "true";
}

bool Script::writeScript(const YAML::Node& outputConfig)
{
    fs::path mainOutputDir(outputConfig["directory"].Scalar());
    if (!fs::exists(mainOutputDir)) {
        fs::create_directory(mainOutputDir);
    }
    std::string outDir = mainOutputDir.string() + fs::path::preferred_separator + outputConfig["binaries"]["mainDir"].Scalar();
    if (!fs::exists(outDir)) {
        fs::create_directory(outDir);
    }
    std::string textDir = outDir + fs::path::preferred_separator + outputConfig["binaries"]["textDir"].Scalar();
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
                        << "incbin " << fs::relative(textDir, mainOutputDir) / binFileName.filename()
                        << endl;
            if (binIt->printpc) {
                includeText << "print pc" << endl;
            }
            tableText[binIt->address] = {false,dataSize,includeText.str()};
            std::ofstream binFile(binFileName.string(), std::ios::binary);
            if (!binFile) {
                std::cerr << "Could not open " << fs::absolute(binFileName.string()).string() << " for writing" << endl;
                return false;
            }
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
                    if (!binFile) {
                        std::cerr << "Could not open " << fs::absolute(binFileName.string()).string() << " for writing" << endl;
                        return false;
                    }
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
        if (!asmFile) {
            std::cerr << "Could not open " << fs::absolute(fs::path(outDir).parent_path() / "textDefines.exp").string() << " for writing" << endl;
            return false;
        } else if (!textDefines) {
            std::cerr << "Could not open " << fs::absolute(fs::path(outDir).parent_path() / "text.asm").string() << " for writing" << endl;
            return false;
        }
        for (auto textIt = tableText.begin(); textIt != tableText.end(); ++textIt) {
            if (textIt->first != lastAddress) {
                int difference = textIt->first - lastAddress;
                if (difference >= 0x10) {
                    if(labels.count(textIt->first) > 0) {
                        std::string& currentLabel = labels[textIt->first];
                        if (difference < 0) {
                            std::cerr << "Warning: " << currentLabel << " will overwrite prevviously assembled data" << endl;
                        }
                        asmFile << endl << "org !def_" << currentLabel << endl;
                        if (firstBankLabel != 0 && (firstBankLabel&0xFF0000) == (textIt->first&0xFF0000)) {
                            textDefines << "!def_" << currentLabel << " = !def_" << labels[firstBankLabel] << "+$" << hex << textIt->first-firstBankLabel << endl;
                        } else {
                            textDefines << "!def_" << currentLabel << " = $" << hex << textIt->first << endl;
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
    return true;
}

bool Script::writeFontData(const fs::path& fontFileName, const YAML::Node& includes)
{
    std::ofstream fontFile(fontFileName.string());
    if (includes.IsDefined() && includes.IsSequence()) {
        for (auto&& include: includes) {
            try {
                std::string&& incFile = include.as<std::string>();
                if (fs::path(incFile).extension().string() == ".bin") {
                    fontFile << "incbin " << incFile << endl;
                } else {
                    fontFile << "incsrc " << incFile;
                    if(fs::path(incFile).extension().string().empty()) {
                        fontFile << ".asm";
                    }
                    fontFile << endl;
                }
            } catch (YAML::BadConversion& e) {
                std::cerr << "Error: " << include.Tag() << " field " << FONT_ADDR << " must be a scalar value." << endl;
                return false;
            }
        }
    }
    for (auto&& it = inConfig.begin(); it != inConfig.end(); ++it) {
        if (it->second[FONT_ADDR].IsDefined()) {
            try {
                std::string define = it->second[FONT_ADDR].Scalar();
                fontFile << endl << "ORG ";
                if (define[0] == '$') {
                   define = define.substr(1);
                }
                try {
                    int address = std::stoi(define, 0, 16);
                    fontFile << '$' << hex << address << endl;
                } catch(std::invalid_argument& e) {
                    if (define[0] != '!') {
                        fontFile << '!';
                    }
                    fontFile << define << endl;
                }
            } catch (YAML::BadConversion& e) {
                std::cerr << "Error: " << it->first.as<std::string>() << " field " << FONT_ADDR << " must be a scalar value." << endl;
                return false;
            }
            int commandValue;
            try {
                commandValue = it->second[CMD_CHAR].as<int>();
            } catch (YAML::BadConversion& e) {
                commandValue = -1;
            }
            int defaultWidth;
            try {
                defaultWidth = it->second[FIXED_WIDTH].as<int>();
            } catch (YAML::BadConversion& e) {
                defaultWidth = -1;
            }
            int byteWidth = it->second[BYTE_WIDTH].as<int>();
            int arraySize;
            if (byteWidth == 2) {
                arraySize = 0xFFFF;
            } else if (byteWidth == 1) {
                arraySize = 0xFF;
            } else {
                std::cerr << "Failed writing widths for " << it->first.Tag() << ": " << BYTE_WIDTH << " must be 1 or 2." << endl;
                return false;
            }
            short* widths = new short[arraySize];
            std::uninitialized_fill(widths, &widths[arraySize], defaultWidth);
            int maxEncoded = 0;
            if (defaultWidth == -1) {
                for (auto encIt = it->second["Encoding"].begin(); encIt != it->second["Encoding"].end(); encIt++) {
                    int code = encIt->second["code"].as<int>();
                    widths[code] = encIt->second["length"].as<short>();
                    if (code > maxEncoded) {
                        maxEncoded = code;
                    }
                }
            } else {
                if (it->second[MAX_CHAR].IsDefined()) {
                    try {
                        maxEncoded = it->second[MAX_CHAR].as<int>();
                    } catch (YAML::BadConversion) {
                        std::cerr << "Error: " << it->first.as<std::string>() << " has an invalid " << MAX_CHAR << endl;
                        return false;
                    }
                } else {
                    for (auto encIt = it->second["Encoding"].begin(); encIt != it->second["Encoding"].end(); encIt++) {
                        int code = encIt->second["code"].as<int>();
                        if (code > maxEncoded) {
                            maxEncoded = code;
                        }
                    }
                }
            }
            int difference = 0;
            int stringLength = 0;
            for (int i = (commandValue == 0 ? 1 : 0) ; i <= maxEncoded; i++) {
                if (widths[i] != -1 && widths[i] != commandValue) {
                    if (difference > 0) {
                        if (stringLength%16 != 0) {
                            unsigned long&& currentPos = fontFile.tellp();
                            fontFile.seekp(currentPos - 1);
                        }
                        stringLength = 0;
                        fontFile << endl << "skip " << std::dec << difference << endl;
                        difference = 0;
                    }
                    if (stringLength == 0) {
                        fontFile << "db ";
                    }
                    fontFile << " $" << setw(2) << std::setfill('0') << hex << widths[i];
                    stringLength++;
                    if (stringLength%16 != 0) {
                        if (i+1 <= maxEncoded) {
                            fontFile << ',';
                        }
                    } else {
                        fontFile << endl;
                        stringLength = 0;
                    }
                } else {
                    difference++;
                }
            }
            delete [] widths;
        }
    }
    fontFile.close();
    return true;
}

Script::operator bool() const
{
    return isScriptValid;
}
