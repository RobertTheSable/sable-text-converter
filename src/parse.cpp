#include "parse.h"
#include "util.h"
#include "exceptions.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <locale>

using sable::TextParser, sable::Font;

TextParser::TextParser(FontList&& list, const std::string& defaultMode, const std::locale& locale) :
    m_FontList(list), defaultFont(defaultMode)
{
    m_Locale = locale;
}

std::pair<bool, int> TextParser::parseLine(std::istream &input, ParseSettings & settings, back_inserter insert, const util::Mapper& mapper)
{
    using boost::locale::boundary::character, boost::locale::boundary::word;
    int length = 0;
    bool finished = false;
    std::string line;
    //const Font& m_FontList[settings.mode] = m_FontList[settings.mode];
    std::istream& state = getline(input, line, '\n');
    if (!state.fail()) {
        if (line.find('\r') != std::string::npos) {
            line.erase(line.find('\r'), 1);
        }

        ssegment_index map(word, line.begin(), line.end(), m_Locale);
        auto it = map.begin();
        bool printNewLine = true;
        bool finishedByComment = false;
        std::string ref = *it;
        while (it++ != map.end() && !finished && !finishedByComment) {
            if (ref == "#") {
                finishedByComment = true;
                if (input.peek() == std::char_traits<char>::eof()) {
                     printNewLine = false;
                }
            } else if (ref == "[") {
                std::string temp;
                {
                    std::string bracketContents = "";
                    for ( ; *it != "]" && it != map.end(); ++it) {
                        bracketContents += *it;
                    }
                    if (it == map.end()) {
                        throw std::runtime_error("Closing bracket not found.");
                    }
                    temp = bracketContents;
                    ref = *(++it);
                }
                {
                    unsigned int code;
                    int bytes;
                    std::tie(code, bytes) = util::strToHex(temp);
                    if (bytes < 0) {
                        bytes = m_FontList[settings.mode].getByteWidth();
                        try {
                            const auto& codeStruct = m_FontList[settings.mode].getCommandData(temp);
                            code = codeStruct.code;
                            finished = (settings.autoend && code == m_FontList[settings.mode].getEndValue());
                            if (m_FontList[settings.mode].getCommandValue() != -1 && !finished) {
                                insertData(m_FontList[settings.mode].getCommandValue(), m_FontList[settings.mode].getByteWidth(), insert);
                            }
                            if (codeStruct.page >= 0) {
                                if (!(codeStruct.page < m_FontList[settings.mode].getNumberOfPages())) {
                                    throw std::runtime_error(
                                        std::string("Page ") + std::to_string(codeStruct.page) + " not found in font " + settings.mode
                                    );
                                }
                                settings.page = codeStruct.page;
                            }
                            printNewLine = !codeStruct.isNewLine;
                        } catch (CodeNotFound &e) {
                            try {
                                std::tie(code, std::ignore) = m_FontList[settings.mode].getTextCode(settings.page, temp);
                                length += m_FontList[settings.mode].getWidth(settings.page, temp);
                            } catch (CodeNotFound &e) {
                                try {
                                    code = m_FontList[settings.mode].getExtraValue(temp);
                                } catch (CodeNotFound &e) {
                                    throw CodeNotFound(temp + " not found in font " + settings.mode);
                                }
                            }
                            finished = settings.autoend && (m_FontList[settings.mode].getCommandValue() == -1) && (code == m_FontList[settings.mode].getEndValue());
                        }
                    }
                    if (!finished) {
                        insertData(code, bytes, insert);
                    }
                }
            } else if (ref == "@") {
                auto startPoint = --it;
                settings = updateSettings(settings, it, map.end(), mapper);
                if (!(settings.page < m_FontList[settings.mode].getNumberOfPages())) {
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
                std::string contents = ref;
                try {
                    auto noun = m_FontList[settings.mode].getNounData(settings.page, contents);
                    while (noun) {
                        insertData(*(noun++), m_FontList[settings.mode].getByteWidth(), insert);
                    }
                } catch (CodeNotFound &e) {
                    auto checkDigraphs = m_FontList[settings.mode].getHasDigraphs();
                    std::string tmpStr = it->str();
                    ssegment_index char_map(character, contents.begin(), contents.end(), m_Locale);


                    ssegment_index next_char_map(character, tmpStr.begin(), tmpStr.end(), m_Locale);
                    std::optional<ssegment_index::iterator> nextCharItr = std::nullopt;
                    std::optional<ssegment_index::iterator> nextCharEnd = std::nullopt;
                    if (tmpStr != "") {
                        nextCharItr = next_char_map.begin();
                        nextCharEnd = next_char_map.end();
                    }
                    for (auto charIt = char_map.begin(); charIt != char_map.end(); charIt++) {
                        std::string currentChar = *charIt, nextChar = "";
                        auto peek = charIt;
                        if (checkDigraphs) {
                            if (++peek != char_map.end()) {
                                nextChar = *peek;
                            } else if (nextCharItr != std::nullopt &&
                                (**nextCharItr != "[" && **nextCharItr != "@" && **nextCharItr != "#")) {
                                nextChar = **nextCharItr;
                            }
                        }
                        unsigned int code;
                        bool advance;
                        // TODO: Canonicalize before checking
                        std::tie<>(code, advance) = m_FontList[settings.mode].getTextCode(settings.page, currentChar, nextChar);
                        if (advance) {
                            if (peek != char_map.end()) {
                                charIt++;
                            } else {
                                if (nextCharItr != std::nullopt && *nextCharItr != *nextCharEnd) {
                                    ++*nextCharItr;
                                }
                            }
                            length += m_FontList[settings.mode].getWidth(settings.page, currentChar + nextChar);
                        } else {
                            length += m_FontList[settings.mode].getWidth(settings.page, currentChar);
                        }
                        insertData(code, m_FontList[settings.mode].getByteWidth(), insert);
                    }
                    if (nextCharItr != std::nullopt) {
                        ref = "";
                        while (*nextCharItr != *nextCharEnd) {
                            ref += (*nextCharItr)->str();
                            ++(*nextCharItr);
                        }
                    }
                }
            }
        }
        finished |= (state.peek() == std::char_traits<char>::eof());
        if (printNewLine && !finished && !state.eof() && input.peek() != '#'  && input.peek() != '@') {
            if (m_FontList[settings.mode].getCommandValue() != -1) {
                insertData(m_FontList[settings.mode].getCommandValue(), m_FontList[settings.mode].getByteWidth(), insert);
            }
            insertData(m_FontList[settings.mode].getCommandCode("NewLine"), m_FontList[settings.mode].getByteWidth(), insert);
        }
        if (settings.autoend && finished) {
            if (m_FontList[settings.mode].getCommandValue() != -1) {
                insertData(m_FontList[settings.mode].getCommandValue(), m_FontList[settings.mode].getByteWidth(), insert);
            }
            insertData(m_FontList[settings.mode].getEndValue(), m_FontList[settings.mode].getByteWidth(), insert);
        }
    } else {
        finished = true;
    }
    return std::make_pair(finished, length);
}

const sable::FontList &TextParser::getFonts() const
{
    return m_FontList;
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
                        } else if (!m_FontList.contains(option)) {
                            throw std::runtime_error(option.insert(0, "Font \"") + "\" was not defined");
                        } else {
                            retVal.mode = option;
                        }
                        if (retVal.maxWidth >= 0) {
                            retVal.maxWidth = m_FontList[retVal.mode].getMaxWidth();
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
    return {true, false, defaultFont, "", m_FontList[defaultFont].getMaxWidth(), address, 0};
}

