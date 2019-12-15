#ifndef SCRIPT_H
#define SCRIPT_H
#include <map>
#include <string>
#include "yaml-cpp/yaml.h"

class Script
{
private:
    struct textNode {
        unsigned int address;
        std::string label;
        bool isMenuText;
        bool storeWidth;
        bool printpc;
        int maxWidth;
        std::vector<unsigned short> data;
    };
    struct asmTextNode {
        bool isTable;
        int length;
        std::string text;
    };

    YAML::Node inConfig, outConfig;
    bool isScriptValid;
    std::vector<textNode> binNodes;
    std::map<int, asmTextNode> tableText;
    std::map<int, std::string> labels;
public:
    Script();
    Script(const char* inConfig, const char* outConfig = nullptr);
    void loadScript(const char* inDir, std::string defaultMode);
    void writeScript(YAML::Node config);
    explicit operator bool() const;
};

#endif
