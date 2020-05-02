#include "parse.h"
#include "util.h"
#include <sstream>
#include <utf8.h>
#include <iostream>
#include <algorithm>
namespace sable {

TextParser::TextParser(const YAML::Node& node, const std::string& defaultMode, util::Mapper mapType) :
    defaultFont(defaultMode), m_RomType(mapType)
    {
        for (auto it = node.begin(); it != node.end(); ++it) {
            m_Fonts[it->first.as<std::string>()] = Font(it->second, it->first.as<std::string>());
        }
    }

    std::pair<bool, int> TextParser::parseLine(std::istream &input, ParseSettings & settings, back_inserter insert)
    {
        int read_count = 0;
        int length = 0;
        bool finished = false;
        std::string line;
        Font& activeFont = m_Fonts.at(settings.mode);
        std::istream& state = getline(input, line, '\n');
        if (!state.fail()) {
            if (line.find('\r') != std::string::npos) {
                line.erase(line.find('\r'), 1);
            }
            auto it = line.begin();
            bool printNewLine = true;
            while (it != line.end() && !finished) {
                std::string currentChar = readUtf8Char(it, line.end());
                if (currentChar == "#") {
                    finished = true;
                    if (it - line.begin() == 1) {
                        printNewLine = false;
                    }
                } else if (currentChar == "[") {
                    std::string temp;
                    {
                        size_t endPt = line.find_first_of(']', it - line.begin());
                        if (endPt == std::string::npos) {
                            throw std::runtime_error("Closing bracket not found.");
                        }
                        size_t distance = (line.begin() +endPt) - it;
                        temp = std::string(it, it+distance);
                        it += distance + 1;
                    }
                    {
                        unsigned int code;
                        int bytes;
                        std::tie(code, bytes) = util::strToHex(temp);
                        if (bytes < 0) {
                            bytes = activeFont.getByteWidth();
                            try {
                                code = activeFont.getCommandCode(temp);
                                finished = (settings.autoend && code == activeFont.getEndValue());
                                if (activeFont.getCommandValue() != -1 && !finished) {
                                    insertData(activeFont.getCommandValue(), activeFont.getByteWidth(), insert);
                                }
                                printNewLine = !activeFont.isCommandNewline(temp);
                            } catch (std::runtime_error &e) {
                                try {
                                    std::tie(code, std::ignore) = activeFont.getTextCode(temp);
                                    length += activeFont.getWidth(temp);
                                } catch (std::runtime_error &e) {
                                    code = activeFont.getExtraValue(temp);
                                }
                                finished = settings.autoend && (activeFont.getCommandValue() == -1) && (code == activeFont.getEndValue());
                            }
                        }
                        if (!finished) {
                            insertData(code, bytes, insert);
                        }
                    }
                } else if (currentChar == "@") {
                    settings = updateSettings(settings, std::string(it, line.end()), settings.currentAddress);
                    if (it - line.begin() == 1 && (state.peek() != std::char_traits<char>::eof())) {
                        return std::make_pair(finished, length);
                    }
                    // Change this to change
                    // activeFont = m_Fonts.at(settings.mode);
                    it = line.end();
                } else {
                    if (settings.currentAddress == 0) {
                        throw std::runtime_error("Attempted to parse text before address was set.");
                    }
                    std::string nextChar = (it == line.end() || !activeFont.getHasDigraphs()) ? "" : readUtf8Char(it, line.end(), false);
                    unsigned int code;
                    bool advance;
                    std::tie<>(code, advance) = activeFont.getTextCode(currentChar, nextChar);
                    if (advance) {
                        utf8::next(it, line.end());
                        length += activeFont.getWidth(currentChar + nextChar);
                    } else {
                        length += activeFont.getWidth(currentChar);
                    }
                    insertData(code, activeFont.getByteWidth(), insert);
                }
            }
            finished |= (state.peek() == std::char_traits<char>::eof());
            if (printNewLine && !finished && !state.eof() && input.peek() != '#'  && input.peek() != '@') {
                if (activeFont.getCommandValue() != -1) {
                    insertData(activeFont.getCommandValue(), activeFont.getByteWidth(), insert);
                }
                insertData(activeFont.getCommandCode("NewLine"), activeFont.getByteWidth(), insert);
            }
            if (settings.autoend && finished) {
                if (activeFont.getCommandValue() != -1) {
                    insertData(activeFont.getCommandValue(), activeFont.getByteWidth(), insert);
                }
                insertData(activeFont.getEndValue(), activeFont.getByteWidth(), insert);
            }
        } else {
            finished = true;
        }
        return std::make_pair(finished, length);
    }

    const std::map<std::string, Font> &TextParser::getFonts() const
    {
        return m_Fonts;
    }

    void TextParser::insertData(unsigned int code, int size, back_inserter bi)
    {
        while (size-- > 0) {
            *(bi++) = (code & 0xFF);
            code >>= 8;
        }
    }

    std::string TextParser::readUtf8Char(std::string::iterator& start, std::string::iterator end, bool advance)
    {
        std::string out("");
        if (advance) {
            utf8::append(utf8::next(start, end), std::back_inserter(out));
        } else {
            utf8::append(utf8::peek_next(start, end), std::back_inserter(out));
        }
        return out;
    }

    ParseSettings TextParser::updateSettings(const ParseSettings &settings, const std::string &setting, unsigned int currentAddress)
    {
        static const std::map<std::string, int> SUPPORTED_SETTINGS = {
            {"printpc", 1},
            {"type", 2},
            {"address", 2},
            {"width", 2},
            {"label", 2},
            {"autoend", 2},
        };
        ParseSettings retVal = settings;
        if (!setting.empty()) {
            std::string name;
            std::istringstream str(setting);
            str >> name;
            if (SUPPORTED_SETTINGS.find(name) == SUPPORTED_SETTINGS.end()) {
                throw std::runtime_error(name.insert(0, "Unrecognized option \"") + '\"');
            } else {
                if (name == "printpc") {
                    retVal.printpc = true;
                } else {
                    std::string option;
                    if (!((str >> option).fail())) {
                        if (name == "type") {
                            if (option == "default") {
                                retVal.mode = defaultFont;
                            } else if (m_Fonts.find(option) == m_Fonts.end()) {
                                throw std::runtime_error(option.insert(0, "Font \"") + "\" was not defined");
                            } else {
                                retVal.mode = option;
                            }
                            if (retVal.maxWidth >= 0) {
                                retVal.maxWidth = m_Fonts[retVal.mode].getMaxWidth();
                            }
                        } else if (name == "address") {
                            if (option != "auto") {
                                auto result = util::strToHex(option);
                                if (result.second >= 0 && util::ROMToPC(m_RomType, result.first) >= 0) {
                                    retVal.currentAddress = util::strToHex(option).first;
                                } else {
                                    throw std::runtime_error(option.insert(0, "Invalid option \"") + "\" for address: must be auto or a SNES address.");
                                }
                            }
                        } else if (name == "width") {
                            if (option == "off") {
                                retVal.maxWidth = -1;
                            } else {
                                if (!(std::istringstream(option) >> std::dec >> retVal.maxWidth)) {
                                    throw std::runtime_error(option.insert(0, "Invalid option \"") + "\" for width: must be off or a decimal number.");
                                }
                            }
                        } else if (name == "label") {
                            retVal.label = option;
                        } else if (name == "autoend") {
                            if (option == "on") {
                                retVal.autoend = true;
                            } else if (option == "off") {
                                retVal.autoend = false;
                            } else {
                                throw std::runtime_error(option.insert(0, "Invalid option \"") + "\" for autoend: must be on or off.");
                            }
                        } else {
                            throw std::runtime_error(name.insert(0, "Unrecognized option \"") + '\"');
                        }
                    } else {
                        throw std::runtime_error(name.insert(0, "Option \"") + "\" is missing a required value");
                    }
                }
            }
        } else {
            throw std::runtime_error("@ symbol found, but no setting specified.");
        }
        return retVal;
    }

    ParseSettings TextParser::getDefaultSetting(int address)
    {
        return {true, false, defaultFont, "", m_Fonts[defaultFont].getMaxWidth(), address};
    }

}
