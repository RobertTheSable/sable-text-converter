#ifndef IMPL_H
#define IMPL_H

#include "project.h"

#include <string>
#include <yaml-cpp/yaml.h>

namespace sable {

struct ProjectSerializer
{
    using Rom = Project::Rom;

    static bool validateConfig(const YAML::Node &configYML);
    static Project read(
        const YAML::Node &config,
        const std::string &projectDir
    );
    static void write(YAML::Node &config, const Project& pr);
};

}

namespace YAML {
    template <>
    struct convert<sable::ProjectSerializer::Rom> {
        static bool decode(const Node& node, sable::ProjectSerializer::Rom& rhs);
    };
}


#endif // IMPL_H
