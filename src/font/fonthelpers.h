#ifndef FONTHELPERS_H
#define FONTHELPERS_H

#include <map>
#include <string>
#include "font.h"

namespace sable {
    bool contains(const std::map<std::string, sable::Font> &f, std::string key);
}

#endif // FONTHELPERS_H
