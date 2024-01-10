#include "normalize.h"
#include <unicode/normalizer2.h>
#include <unicode/bytestream.h>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <iostream>

namespace sable {


std::string normalize(const std::string &in)
{
    UErrorCode err = U_ZERO_ERROR;
    std::string sinkDestNFD, sinkDestNFC;
    icu::StringByteSink sinkNFD(&sinkDestNFD);
    icu::StringByteSink sinkNFC(&sinkDestNFC);

    if (auto instance = icu::Normalizer2::getNFDInstance(err); U_FAILURE(err)) {
        throw std::runtime_error("couldn't get the NFD instance.");
    } else if (instance->normalizeUTF8(0, in, sinkNFD, nullptr, err); U_FAILURE(err)) {
        throw std::runtime_error("cound't perform NFD normalization.");
    }

    if (auto instance = icu::Normalizer2::getNFCInstance(err); U_FAILURE(err)) {
        throw std::runtime_error("couldn't get the NFC instance.");
    } else if (instance->normalizeUTF8(0, sinkDestNFD, sinkNFC, nullptr, err); U_FAILURE(err)) {

        throw std::runtime_error("cound't perform NFC normalization.");
    }

    return sinkDestNFC;
}

} // namespace sable
