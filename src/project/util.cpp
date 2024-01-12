#include "util.h"

#include <algorithm>
#include <sstream>

#include "data/mapper.h"

size_t sable::util::calculateFileSize(const std::string &value)
{
    size_t returnVal = 0;
    if (!value.empty()) {
        std::istringstream stream(value);
        uint64_t calculatedValue = 0;
        if (stream >> calculatedValue) {
            calculatedValue *= 1024;
            if (!stream.eof()) {
                if (stream.peek() == '.') {
                    stream.get();
                    uint64_t tempValue = 0;
                    if (stream >> tempValue) {
                        calculatedValue += (tempValue * 1024) / 10;
                    }
                }
                std::string sizeIndicator = "";
                if (stream >> sizeIndicator) {
                    std::transform(sizeIndicator.begin(), sizeIndicator.end(), sizeIndicator.begin(), ::tolower);
                    if (sizeIndicator == "kb" || sizeIndicator == "k") {
                        calculatedValue = calculatedValue * 1024;
                    } else if ((sizeIndicator == "mb") || (sizeIndicator == "m")) {
                        calculatedValue = calculatedValue * (1024 * 1024);
                    } else {
                        // other sizes not supported.
                        calculatedValue = 0;
                    }
                }
                if (stream >> sizeIndicator) {
                    calculatedValue = 0;
                }
            }
            if (calculatedValue != 0 && (calculatedValue % (1024 * 1024)) == 0) {
                calculatedValue /= 1024;
                if (calculatedValue <= MAX_ALLOWED_FILESIZE_SHORTCUT) {
                    returnVal = calculatedValue;
                }
            }
        }
    }
    return returnVal;
}

std::string sable::util::getFileSizeString(int value)
{
    if (value > MAX_ALLOWED_FILESIZE_SHORTCUT || value <= 0) {
        return ""; // not covered
    } else if (value >= ROM_MAX_SIZE) {
        value = MAX_ALLOWED_FILESIZE_SHORTCUT;
    }
    int returnVal = value / 1024;
    if (returnVal < 1024) {
        return std::to_string(returnVal)+ "kb";
    }
    returnVal /= 1024;
    if ((value/1024) % 1024 == 512) {
        return std::to_string(returnVal) + ".5mb";
    } else {
        return std::to_string(returnVal)+ "mb";
    }

}
