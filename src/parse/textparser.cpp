#include "parse.h"
#include <sstream>
#include <iostream>
#include <map>
#include <array>

#include <unicode/uchar.h>

#include "unicode.h"
#include "data/optionhelpers.h"

using sable::TextParser, sable::Font;

struct TextParser::Impl {
    std::string defaultFont;
    std::map<std::string, sable::Font> fontList;
    icu::Locale m_Locale;
    bool useDigraphs;
    int maxWidth;
    Impl(
        const std::string& defFont,
        std::map<std::string, sable::Font>&& fList,
        icu::Locale&& locale
    )
        : defaultFont{defFont}, fontList{fList}, m_Locale{locale} {}
    ~Impl() {

    }

    ParseSettings updateSettings(
        const ParseSettings &settings,
        BreakIterator it,
        const util::Mapper& mapper
    ) {
        static const std::array<const char*, 10> SUPPORTED_SETTINGS{
            "printpc",
            "type",
            "address",
            "width",
            "label",
            "autoend",
            "page",
            "end_on_label",
            "export_width",
            "export_address"
        };
        ParseSettings retVal = settings;
        if (!it.done()) {
            std::string name = "";
            // Trying to mimic behavior of a stream WRT whitespace
            for (++it; !it.done() && !u_isspace(it.ufront()) ; ++it) {
                name += *it;
            }
            if (name == "") {
                throw std::runtime_error("@ symbol found, but no setting specified.");
            }
            if (std::find(SUPPORTED_SETTINGS.begin(), SUPPORTED_SETTINGS.end(), name) == SUPPORTED_SETTINGS.end()) {
                throw std::runtime_error(name.insert(0, "Unrecognized option \"") + '\"');
            } else {
                if (name == "printpc") {
                    retVal.printpc = true;
                } else {
                    std::string option = "";
                    if (!it.done()) {
                        for (++it; !it.done() && !u_isspace(it.ufront()) ; ++it) {
                            option += *it;
                        }
                    }
                    if (option != "") {
                        if (name == "type") {
                            if (option == "default") {
                                retVal.mode = defaultFont;
                            } else if (fontList.find(option) == fontList.end()) {
                                throw std::runtime_error(option.insert(0, "Font \"") + "\" was not defined");
                            } else {
                                retVal.mode = option;
                            }
                            if (retVal.maxWidth >= 0) {
                                retVal.maxWidth = fontList[retVal.mode].getMaxWidth();
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
                            retVal.autoend = options::parseBool<ParseSettings::Autoend>(name, option);
                        } else if (name == "page") {
                            try {
                                int page = std::stoi(option);
                                retVal.page = page;
                            } catch (std::invalid_argument &e) {
                                throw std::runtime_error(option.insert(0, "Page \"") + "\" is not a decimal integer.");
                            }
                        } else if (name == "end_on_label") {
                            retVal.endOnLabel = options::parseBool<ParseSettings::EndOnLabel>(name, option);
                        } else if (name == "export_width") {
                            retVal.exportWidth = options::parseBool<sable::options::ExportWidth>(name, option);
                        } else if (name == "export_address") {
                            retVal.exportAddress = options::parseBool<sable::options::ExportAddress>(name, option);
                        } else {
                            // should be unreachable
                            throw std::runtime_error(name.insert(0, "Unrecognized option \"") + '\"');
                        }
                    } else {
                        throw std::runtime_error(name.insert(0, "Option \"") + "\" is missing a required value");
                    }
                }
            }
        } else {
            //probably unreachable
            throw std::runtime_error("@ symbol found, but no setting specified.");
        }
        return retVal;
    }
    static void insertData(unsigned int code, int size, back_inserter bi)
    {
        while (size-- > 0) {
            *(bi++) = (code & 0xFF);
            code >>= 8;
        }
    }
};

TextParser::TextParser(
        std::map<std::string,
        sable::Font>&& list,
        const std::string& defaultMode,
        const std::string& locale,
        options::ExportWidth defaultExportWidth,
        options::ExportAddress defaultExportAddress
        ) : defaultExportWidth_{defaultExportWidth}, defaultExportAddress_{defaultExportAddress}
{
    _pImpl = std::make_unique<Impl>(
        defaultMode,
        std::move(list),
        icu::Locale::createCanonical(locale.c_str())
    );
}

TextParser::~TextParser()=default;

TextParser::Result TextParser::parseLine(
        std::istream &input,
        ParseSettings & settings,
        back_inserter insert,
        Metadata lastReadWasMetadata,
        const util::Mapper& mapper)
{
    int length = 0;
    bool finished = false;
    std::string line{""};
    std::istream& state = getline(input, line, '\n');
    auto label = settings.label;
    Metadata mt = Metadata::No;

    auto insertCommand = [&insert = insert, &_pImpl = _pImpl, &font = _pImpl->fontList[settings.mode]] (std::string code)
    {
        if (font.getCommandValue() != -1) {
            _pImpl->insertData(font.getCommandValue(), font.getByteWidth(), insert);
        }
        if (code == "") {
            _pImpl->insertData(font.getEndValue(), font.getByteWidth(), insert);
        } else {
            _pImpl->insertData(font.getCommandCode(code), font.getByteWidth(), insert);
        }

    };

    if (!state.fail()) {
        if (line.find('\r') != std::string::npos) {
            line.erase(line.find('\r'), 1);
        }

        BreakIterator it(true, line, _pImpl->m_Locale);
        bool printNewLine = true;
        bool finishedByComment = false;
        std::string ref = *it;
        while (!(it++.done()) && !finished && !finishedByComment) {
            if (ref == "#") {
                finishedByComment = true;
                if (input.peek() == std::char_traits<char>::eof()) {
                     printNewLine = false;
                }
                mt = Metadata::Yes;
            } else if (ref == "[") {
                std::string temp;
                {
                    std::string bracketContents = "";
                    for ( ; *it != "]" && !it.done(); ++it) {
                        bracketContents += *it;
                    }
                    if (it.done()) {
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
                        bytes = _pImpl->fontList[settings.mode].getByteWidth();
                        try {
                            const auto& codeStruct = _pImpl->fontList[settings.mode].getCommandData(temp);
                            code = codeStruct.code;
                            finished = (options::isEnabled(settings.autoend) &&
                                        code == _pImpl->fontList[settings.mode].getEndValue()
                                    );
                            if (_pImpl->fontList[settings.mode].getCommandValue() != -1 && !finished) {
                                _pImpl->insertData(_pImpl->fontList[settings.mode].getCommandValue(), _pImpl->fontList[settings.mode].getByteWidth(), insert);
                            }
                            if (codeStruct.page >= 0) {
                                if (!(codeStruct.page < _pImpl->fontList[settings.mode].getNumberOfPages())) {
                                    throw std::runtime_error(
                                        std::string("Page ") + std::to_string(codeStruct.page) + " not found in font " + settings.mode
                                    );
                                }
                                settings.page = codeStruct.page;
                            }
                            printNewLine = !codeStruct.isNewLine;
                        } catch (CodeNotFound &e) {
                            try {
                                std::tie(code, std::ignore) = _pImpl->fontList[settings.mode].getTextCode(settings.page, temp);
                                length += _pImpl->fontList[settings.mode].getWidth(settings.page, temp);
                            } catch (CodeNotFound &e) {
                                try {
                                    code = _pImpl->fontList[settings.mode].getExtraValue(temp);
                                } catch (CodeNotFound &e) {
                                    throw CodeNotFound(temp + " not found in font " + settings.mode);
                                }
                            }
                            if (auto font = _pImpl->fontList[settings.mode];
                                font.getCommandValue() == -1 &&
                                code == font.getEndValue()
                            ) {
                                finished |= options::isEnabled(settings.autoend);
                            }
                        }
                    }

                    if (auto font = _pImpl->fontList[settings.mode];
                        font.getCommandValue() == -1 &&
                        code == font.getEndValue()
                    ) {
                        finished |= options::isEnabled(settings.autoend);
                    }

                    if (!finished) {
                         _pImpl->insertData(code, bytes, insert);
                    }
                }
            } else if (ref == "@") {
                mt = Metadata::Yes;
                auto startPoint = --it;
                settings = _pImpl->updateSettings(settings, it, mapper);
                if (!(settings.page < _pImpl->fontList[settings.mode].getNumberOfPages())) {
                    throw std::runtime_error(
                        std::string("Page ") + std::to_string(settings.page) + " not found in font " + settings.mode
                    );
                }
                if (settings.label != label) {
                    finished |= options::isEnabled(settings.endOnLabel);
                }
                if (it.atStart() && (state.peek() != std::char_traits<char>::eof())) {
                    return Result{
                        finished,
                        length,
                        label,
                        mt
                    };
                }
                while (!it.done()) {
                    ++it;
                }
            } else {
                if (settings.currentAddress == 0) {
                    throw std::runtime_error("Attempted to parse text before address was set.");
                } else if (mapper.ToPC(settings.currentAddress) == -1) {
                    std::ostringstream err;
                    err << "Attempted to begin parsing with invalid ROM address $" << std::hex << settings.currentAddress;
                    throw std::runtime_error(err.str());
                }
                std::string contents = ref;
                try {
                    auto noun = _pImpl->fontList[settings.mode].getNounData(settings.page, contents);
                    while (noun) {
                        _pImpl->insertData(*(noun++), _pImpl->fontList[settings.mode].getByteWidth(), insert);
                    }
                } catch (CodeNotFound &e) {
                    auto checkDigraphs = _pImpl->fontList[settings.mode].getHasDigraphs();
                    std::string tmpStr = *it;
                    BreakIterator charIt (false, contents, _pImpl->m_Locale);

                    std::optional<BreakIterator> nextCharItr = std::nullopt;
                    if (tmpStr != "") {
                        nextCharItr = BreakIterator(false, tmpStr, _pImpl->m_Locale);
                    }
                    for ( ;!charIt.done(); ++charIt) {
                        std::string currentChar = *charIt, nextChar = "";
                        auto peek = charIt;
                        if (checkDigraphs) {
                            if (!(++peek).done()) {
                                nextChar = *peek;
                            } else if (nextCharItr != std::nullopt &&
                                (**nextCharItr != "[" && **nextCharItr != "@" && **nextCharItr != "#")) {
                                nextChar = **nextCharItr;
                            }
                        }
                        unsigned int code;
                        bool advance;

                        std::tie<>(code, advance) = _pImpl->fontList[settings.mode].getTextCode(settings.page, currentChar, nextChar);
                        if (advance) {
                            if (!peek.done()) {
                                ++charIt;
                            } else {
                                if (nextCharItr != std::nullopt && !nextCharItr->done()) {
                                    ++(*nextCharItr);
                                }
                            }
                            length += _pImpl->fontList[settings.mode].getWidth(settings.page, currentChar + nextChar);
                        } else {
                            length += _pImpl->fontList[settings.mode].getWidth(settings.page, currentChar);
                        }
                        _pImpl->insertData(code, _pImpl->fontList[settings.mode].getByteWidth(), insert);
                    }
                    if (nextCharItr != std::nullopt) {
                        ref = "";
                        while (!nextCharItr->done()) {
                            ref += **nextCharItr;
                            ++(*nextCharItr);
                        }
                    }
                }
            }
        }

        if (printNewLine &&
                !finished &&
                state.peek() != std::char_traits<char>::eof() &&
                !state.eof() &&
                input.peek() != '#'  &&
                input.peek() != '@') {
            insertCommand("NewLine");
        }
        if (finished ||
            (mt == Metadata::No && state.peek() == std::char_traits<char>::eof())
        ) {
            if (options::isEnabled(settings.autoend)) {
                insertCommand("");
            }
            finished = true;
        }
    } else {
        finished = true;
        if (options::isEnabled(settings.autoend) && lastReadWasMetadata == Metadata::Yes) {
            insertCommand("");
        }
    } // !state.fail()

    return Result{
        finished,
        length,
        label,
        mt
    };
}

const std::map<std::string, sable::Font> &TextParser::getFonts() const
{
    return _pImpl->fontList;
}

auto TextParser::getDefaultSetting(int address) const -> ParseSettings
{
    auto def = _pImpl->defaultFont;
    return {
        ParseSettings::Autoend::On,
        false,
        def,
        "",
        _pImpl->fontList[def].getMaxWidth(),
        address,
        0,
        ParseSettings::EndOnLabel::Off,
        defaultExportAddress_,
        defaultExportWidth_
    };
}

