#ifndef SABLE_OPTIONS_H
#define SABLE_OPTIONS_H 

#include "yaml-cpp/yaml.h"
#include "wrapper/filesystem.h"

namespace ParseOptions {
    static const int nothingToDo = 0;
    static const int printHelp = 1;
    static const int assembleRoms = 2;
    static const int convertScript = 4;
    static const int verbose = 8;
    static const int quiet = 16;
}

bool validateConfig(const YAML::Node& configYML);
int parseOptions(int argc, char * argv[]);

#endif
