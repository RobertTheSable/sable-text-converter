#ifndef MAPPERCONV_H
#define MAPPERCONV_H

#include <yaml-cpp/yaml.h>
#include "util.h"

namespace YAML {
    template <>
    struct convert<sable::util::MapperType> {
        static bool decode(const Node& node, sable::util::MapperType& rhs);
    };
}

#endif // MAPPERCONV_H
