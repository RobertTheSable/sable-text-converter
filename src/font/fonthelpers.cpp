#include "fonthelpers.h"

bool sable::contains(const std::map<std::string, Font> &f, std::string key)
{
    return f.find(key) != f.end();
}
