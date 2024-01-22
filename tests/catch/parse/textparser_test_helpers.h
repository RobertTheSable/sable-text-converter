#ifndef TEXTPARSER_TEST_HELPERS_H
#define TEXTPARSER_TEST_HELPERS_H

#include <sstream>
#include <iostream>

#include "parse/textparser.h"
#include "font/font.h"
#include "font/fonthelpers.h"
#include "helpers.h"
#include "data/optionhelpers.h"


typedef std::vector<unsigned char> ByteVector;

namespace {
    const auto jpLocale = "ja_JP.UTF-8";
    const auto defLocale = "en_US.UTF-8";
}
using sable::options::ExportAddress,
sable::options::ExportWidth,
sable::ParseSettings;
using Metadata = sable::TextParser::Metadata;

inline bool operator==(sable::TextParser::Result lhs, std::pair<bool, int> rhs)
{
    return lhs.endOfBlock == rhs.first && lhs.length == rhs.second;
}

#endif // TEXTPARSER_TEST_HELPERS_H
