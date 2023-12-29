#include "mapperconv.h"

#include <algorithm>


bool YAML::convert<sable::util::MapperType>::decode(const YAML::Node &node, sable::util::MapperType &rhs)
{
    using sable::util::MapperType;
    std::string stringVal = node.as<std::string>();
    std::transform(stringVal.begin(), stringVal.end(), stringVal.begin(), ::tolower);
    if (stringVal == "lorom") {
        rhs = MapperType::LOROM;
    } else if (stringVal == "hirom") {
        rhs = MapperType::HIROM;
    } else if (stringVal == "exlorom") {
        rhs = MapperType::EXLOROM;
    } else if (stringVal == "exhirom") {
        rhs = MapperType::EXHIROM;
    } else {
        rhs = MapperType::INVALID;
        return false;
    }
    return true;
}
