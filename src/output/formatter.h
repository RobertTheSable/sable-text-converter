#ifndef SABLE_FORMATTER_H
#define SABLE_FORMATTER_H

#include <string>
#include <type_traits>
#include <iostream>
#include <sstream>
#include <bitset>

#include <wrapper/filesystem.h>
namespace sable {

namespace formatter {

std::string generateDefine(const std::string& label);
std::string generateInclude(const fs::path &file, const fs::path &basePath, bool isBin);

template<typename I>
std::enable_if_t<std::is_integral_v<I>, std::string> generateNumber(I number, int width, int base = 16)
{
    std::ostringstream output;
    if (base == 16) {
        if (width > 4 || width <= 0) {
            // Asar does support 4-byte data even though the SNES doesn't really.
            throw std::runtime_error(std::string("Unsupported width ") +  std::to_string(width));
        }
        if (width == 3) {
            number = (number & 0xFFFFFF);
        } else if (width == 2) {
            number = (number & 0xFFFF);
        } else if (width == 1) {
            number = (number & 0xFF);
        }
        output << '$' << std::setfill('0') << std::setw(width *2) << std::hex << number;
    } else if (base == 10) {
        output << std::dec << number;
    } else if (base == 2) {
        if (width == 4) {
            output << '%' + std::bitset<32>(number).to_string();
        } else if (width == 3) {
            output << '%' + std::bitset<24>(number).to_string();
        } else if (width == 2) {
            output << '%' + std::bitset<16>(number).to_string();
        } else if (width == 1) {
            output << '%' + std::bitset<8>(number).to_string();
        } else {
            throw std::runtime_error(std::string("Unsupported width ") +  std::to_string(width));
        }

    } else {
        throw std::runtime_error(std::string("Unsupported base ") +  std::to_string(base));
    }
    return output.str();
}

template<typename I>
std::enable_if_t<std::is_integral_v<I>, std::string> generateAssignment(
    const std::string& label,
    I value,
    int width,
    int base = 16,
    const std::string& baseLabel = ""
)
{
    std::ostringstream output;
    output << generateDefine(label) <<  " = ";
    if (!baseLabel.empty()) {
        output << generateDefine(baseLabel);
        if (value != 0) {
            output << '+' + generateNumber(value, width, base);
        }
    } else {
        output << generateNumber(value, width, base);
    }

    return output.str();
}


}
} // namespace sable

#endif // SABLE_FORMATTER_H
