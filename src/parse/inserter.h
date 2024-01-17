#ifndef SABLE_INSERTER_H
#define SABLE_INSERTER_H

#include "font/font.h"
#include "settings.h"
#include "data/optionhelpers.h"
#include "data/mapper.h"

namespace sable {

template<class Insert>
class Inserter
{
    const sable::Font& font_;
    sable::ParseSettings& settings_;
    Insert insert_;

public:
    Inserter(const sable::Font& font, sable::ParseSettings& settings, Insert insert)
        : font_{font}, settings_{settings}, insert_{insert}
    {

    }

    void insertData(unsigned int code, int size)
    {
        while (size-- > 0) {
            *(insert_++) = (code & 0xFF);
            code >>= 8;
        }
    }

    struct BracketResult {
        bool finished, printNewLine;
        int length;
    };

    std::optional<BracketResult> insertCommand (const std::string& code)
    {
         try {
            auto& cmd = font_.getCommandData(code);

            if (cmd.page >= 0) {
                if (!(cmd.page < font_.getNumberOfPages())) {
                    throw std::runtime_error(
                        std::string("Page ") + std::to_string(cmd.page) + " not found in font " + settings_.mode
                    );
                }
                settings_.page = cmd.page;
            }

            if (auto cv = font_.getCommandValue(); cv && cmd.isPrefixed) {
                insertData(cv.value(), font_.getByteWidth());
            }
            insertData(cmd.code, font_.getByteWidth());
            return BracketResult {
                (options::isEnabled(settings_.autoend) && cmd.code == font_.getEndValue()),
                !cmd.isNewLine,
                0
            };
        } catch (CodeNotFound& e) {
            return std::nullopt;
        }

    }

    BracketResult handleBrackets(const std::string &contents, bool printNewLine)
    {
        BracketResult r;

        unsigned int code = font_.getEndValue();
        int width = 0;
        bool needNewLine = printNewLine;
        if (auto result = util::strToHex(contents); (bool)result) {
            insertData(result->value, result->length);
            code = result->value;
        } else if (auto r = insertCommand(contents); (bool)r) {
            return *r;
        } else if (auto t = font_.getTextCode(settings_.page, contents); (bool)t) {
            code = std::get<0>(t.value());
            insertData(code, font_.getByteWidth());
            width = font_.getWidth(settings_.page, contents);
            needNewLine = true;
        } else if (auto ex = font_.getExtraValue(contents); (bool)ex) {
            insertData(ex.value(), font_.getByteWidth());
            code = ex.value();
        } else {
            throw CodeNotFound(contents + " not found in font " + settings_.mode);
        }

        return BracketResult {
            options::isEnabled(settings_.autoend) && !font_.getCommandValue() && code == font_.getEndValue(),
            needNewLine,
            width
        };
    }
};

} // namespace sable

#endif // SABLE_INSERTER_H
