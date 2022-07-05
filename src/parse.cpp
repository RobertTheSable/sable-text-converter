#include "parse.h"
#include "util.h"
#include "exceptions.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <locale>

using sable::TextParser, sable::Font;

TextParser::TextParser(const YAML::Node& node, const std::string& defaultMode, const std::string& localeName) :
    defaultFont(defaultMode)
{
    m_Locale =  boost::locale::generator().generate(localeName);
    for (auto it = node.begin(); it != node.end(); ++it) {
        m_Fonts[it->first.as<std::string>()] = Font(it->second, it->first.as<std::string>());
    }
}

std::pair<bool, int> TextParser::parseLine(std::istream &input, ParseSettings & settings, back_inserter insert, const util::Mapper& mapper)
{
    using boost::locale::boundary::character, boost::locale::boundary::word;
    int length = 0;
    bool finished = false;
    std::string line;
    Font& activeFont = m_Fonts.at(settings.mode);
    std::istream& state = getline(input, line, '\n');
    if (!state.fail()) {
        if (line.find('\r') != std::string::npos) {
            line.erase(line.find('\r'), 1);
        }

        ssegment_index map(word, line.begin(), line.end(), m_Locale);
        auto it = map.begin();
        bool printNewLine = true;
        bool finishedByComment = false;
        while (it != map.end() && !finished && !finishedByComment) {
            if (*it == "#") {
                finishedByComment = true;
                if (input.peek() == std::char_traits<char>::eof()) {
                     printNewLine = false;
                }
            } else if (*it == "[") {
                std::string temp;
                {
                    std::string bracketContents = "";
                    for (it++ ; *it != "]" && it != map.end(); it++) {
                        bracketContents += *it;
                    }
                    if (it == map.end()) {
                        throw std::runtime_error("Closing bracket not found.");
                    }
                    temp = bracketContents;
                    it++;
                }
                {
                    unsigned int code;
                    int bytes;
                    std::tie(code, bytes) = util::strToHex(temp);
                    if (bytes < 0) {
                        bytes = activeFont.getByteWidth();
                        try {
                            const auto& codeStruct = activeFont.getCommandData(temp);
                            code = codeStruct.code;
                            finished = (settings.autoend && code == activeFont.getEndValue());
                            if (activeFont.getCommandValue() != -1 && !finished) {
                                insertData(activeFont.getCommandValue(), activeFont.getByteWidth(), insert);
                            }
                            if (codeStruct.page >= 0) {
                                if (!(codeStruct.page < activeFont.getNumberOfPages())) {
                                    throw std::runtime_error(
                                        std::string("Page ") + std::to_string(codeStruct.page) + " not found in font " + settings.mode
                                    );
                                }
                                settings.page = codeStruct.page;
                            }
                            printNewLine = !codeStruct.isNewLine;
                        } catch (CodeNotFound &e) {
                            try {
                                std::tie(code, std::ignore) = activeFont.getTextCode(settings.page, temp);
                                length += activeFont.getWidth(settings.page, temp);
                            } catch (CodeNotFound &e) {
                                try {
                                    code = activeFont.getExtraValue(temp);
                                } catch (CodeNotFound &e) {
                                    throw CodeNotFound(temp + " not found in font " + settings.mode);
                                }
                            }
                            finished = settings.autoend && (activeFont.getCommandValue() == -1) && (code == activeFont.getEndValue());
                        }
                    }
                    if (!finished) {
                        insertData(code, bytes, insert);
                    }
                }
            } else if (*it == "@") {
                auto startPoint = it;
                settings = updateSettings(settings, it, map.end(), mapper);
                if (!(settings.page < activeFont.getNumberOfPages())) {
                    throw std::runtime_error(
                        std::string("Page ") + std::to_string(settings.page) + " not found in font " + settings.mode
                    );
                }
                if (startPoint == map.begin() && (state.peek() != std::char_traits<char>::eof())) {
                    return std::make_pair(finished, length);
                }
                it = map.end();
            } else {
                if (settings.currentAddress == 0) {
                    throw std::runtime_error("Attempted to parse text before address was set.");
                }
                std::string contents = (it++)->str();
                try {
                    auto noun = activeFont.getNounData(settings.page, contents);
                    while (noun) {
                        insertData(*(noun++), activeFont.getByteWidth(), insert);
                    }
                } catch (CodeNotFound &e) {
                    ssegment_index char_map(character, contents.begin(), contents.end(), m_Locale);
                    for (auto charIt = char_map.begin(); charIt != char_map.end(); charIt++) {
                        std::string currentChar = *charIt, nextChar = "";
                        auto peek = charIt;
                        if (activeFont.getHasDigraphs()) {
                            if (++peek != char_map.end()) {
                                nextChar = *peek;
                            } else if (it != map.end() && it->rule() & boost::locale::boundary::word_none) {
                                // we've reached the end of a word, so we need to grab the character from the next word.
                                // can't just grab one character because of unicode

                                // for some reason, using it->str without copying it on windows makes ICU throw an error
                                // who knows why :v
                                std::string tmpStr = it->str();
                                nextChar = ssegment_index(character, tmpStr.begin(), tmpStr.end(), m_Locale).begin()->str();

                            }
                        }
                        unsigned int code;
                        bool advance;
                        std::tie<>(code, advance) = activeFont.getTextCode(settings.page, currentChar, nextChar);
                        if (advance) {
                            if (peek == char_map.end()) {
                                it++;
                            } else {
                                charIt++;
                            }
                            length += activeFont.getWidth(settings.page, currentChar + nextChar);
                        } else {
                            length += activeFont.getWidth(settings.page, currentChar);
                        }
                        insertData(code, activeFont.getByteWidth(), insert);
                    }
                }
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

using sable::ParseSettings;

sable::ParseSettings TextParser::updateSettings(const ParseSettings &settings, ssegment_index::iterator& it, const ssegment_index::const_iterator& end, const util::Mapper& mapper)
{
    static const std::map<std::string, int> SUPPORTED_SETTINGS = {
        {"printpc", 1},
        {"type", 2},
        {"address", 2},
        {"width", 2},
        {"label", 2},
        {"autoend", 2},
        {"page", 2},
    };
    ParseSettings retVal = settings;
    if (it != end) {
        std::string name = "";
        using namespace boost::locale::boundary;
        // Trying to mimic behavior of a stream WRT whitespace
        for (it++; it != end && !std::isspace(it->str().front(), m_Locale) ; it++) {
            name += *it;
        }
        if (SUPPORTED_SETTINGS.find(name) == SUPPORTED_SETTINGS.end()) {
            throw std::runtime_error(name.insert(0, "Unrecognized option \"") + '\"');
        } else {
            if (name == "printpc") {
                retVal.printpc = true;
            } else {
                std::string option = "";
                if (it != end) {
                    for (it++; it != end &&  !std::isspace(it->str().front(), m_Locale) ; it++) {
                        option += *it;
                    }
                }
                if (option != "") {
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
                            int convertedAddress = mapper.ToPC(result.first);
                            if (result.second >= 0 && mapper.ToPC(result.first) >= 0) {
                                retVal.currentAddress = util::strToHex(option).first;
                            } else {
                                if (convertedAddress == -2) {
                                    throw std::runtime_error(option.insert(0, "Invalid option \"") + "\" for address: address is too large for the specified ROM size.");
                                } else {
                                    // don't need to handle -2 case since it won't happen when given an SNES-style address to start with.
                                    throw std::runtime_error(option.insert(0, "Invalid option \"") + "\" for address: must be auto or a SNES address.");
                                }
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
                    } else if (name == "page") {
                        try {
                            int page = std::stoi(option);
                            retVal.page = page;
                        } catch (std::invalid_argument &e) {
                            throw std::runtime_error(option.insert(0, "Page \"") + "\" is not a decimal integer.");
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
    return {true, false, defaultFont, "", m_Fonts[defaultFont].getMaxWidth(), address, 0};
}

