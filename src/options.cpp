#include "options.h"
#include <iostream>

using namespace std;

bool validateConfig(const YAML::Node& configYML) {
    bool isValid = true;
    if (!configYML["files"].IsDefined() || !configYML["files"].IsMap()) {
        isValid = false;
        cerr << "Error: files section is missing or not a map.\n";
    } else {
        string mainDir = "";
        if (configYML["files"]["mainDir"].IsDefined() && configYML["files"]["mainDir"].IsScalar()) {
            mainDir = configYML["files"]["mainDir"].Scalar();
        }
        if (!configYML["files"]["input"].IsDefined() || !configYML["files"]["input"].IsMap()) {
            isValid = false;
            cerr << "Error: input section is missing or not a map.\n";
        } else {
            if (!configYML["files"]["input"]["directory"].IsDefined() || !configYML["files"]["input"]["directory"].IsScalar()) {
                isValid = false;
                cerr << "Error: input directory for project is missing from files config.\n";
            } else {
                fs::path textPath = fs::path(mainDir) / configYML["files"]["input"]["directory"].Scalar();
                if (!fs::exists(textPath)) {
                    isValid = false;
                    cerr << "Error: input directory " << textPath << " does not exist.\n";
                }
            }
        }
        if (!configYML["files"]["output"].IsDefined() || !configYML["files"]["output"].IsMap()) {
            isValid = false;
            cerr << "Error: input section is missing or not a map.\n";
        } else {
            if (!configYML["files"]["output"]["directory"].IsDefined() || !configYML["files"]["output"]["directory"].IsScalar()) {
                isValid = false;
                cerr << "Error: output directory for project is missing from files config.\n";
            }
            if (!configYML["files"]["output"]["binaries"].IsDefined() || !configYML["files"]["output"]["binaries"].IsMap()) {
                isValid = false;
                cerr << "Error: output binaries subdirectory section is missing or not a map.\n";
            } else {
                if (!configYML["files"]["output"]["binaries"]["mainDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["mainDir"].IsScalar()) {
                    isValid = false;
                    cerr << "Error: output binaries subdirectory mainDir value is missing from files config.\n";
                }
                if (!configYML["files"]["output"]["binaries"]["textDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["textDir"].IsScalar()) {
                    isValid = false;
                    cerr << "Error: output binaries subdirectory textDir value is missing from files config.\n";
                }
                if (configYML["files"]["output"]["binaries"]["extras"].IsDefined() && !configYML["files"]["output"]["binaries"]["extras"].IsSequence()) {
                    isValid = false;
                    cerr << "Error: extras section for output binaries must be a sequence.\n";
                }
            }
            if (configYML["files"]["output"]["includes"].IsDefined() && !configYML["files"]["output"]["includes"].IsSequence()) {
                isValid = false;
                cerr << "Error: includes section for output must be a sequence.\n";
            }
        }
        if (!configYML["files"]["romDir"].IsDefined() || !configYML["files"]["romDir"].IsScalar()) {
            isValid = false;
            cerr << "Error: romDir for project is missing from files config.\n";
        } else {
            if (!fs::exists(configYML["files"]["romDir"].Scalar())) {
                isValid = false;
                cerr << "Error: directory \"" << configYML["files"]["romDir"].Scalar() << "\" does not exist.\n";
            }
        }
    }
    if (!configYML["config"].IsDefined() || !configYML["config"].IsMap()) {
        isValid = false;
        cerr << "Error: config section is missing or not a map.\n";
    } else {
        if (!configYML["config"]["directory"].IsDefined() || !configYML["config"]["directory"].IsScalar()) {
            isValid = false;
            cerr << "Error: directory for config section is missing or is not a scalar.\n";
        }
        if (!configYML["config"]["inMapping"].IsDefined() || !configYML["config"]["inMapping"].IsScalar()) {
            isValid = false;
            cerr << "Error: inMapping for config section is missing or is not a scalar.\n";
        }
        if (configYML["config"]["mapper"].IsDefined() && !configYML["config"]["mapper"].IsScalar()) {;
            isValid = false;
            cerr << "Error: config > mapper must be a string.\n";
        }
    }
    if (!configYML["roms"].IsDefined()) {
        isValid = false;
        cerr << "Error: roms section is missing.\n";
    } else {
        if (!configYML["roms"].IsSequence()) {
            isValid = false;
            cerr << "Error: roms section in config must be a sequence.\n";
        } else {
            int romindex = 0;
            for(auto node: configYML["roms"]) {
                string romName;
                if (!node["name"].IsDefined() || !node["name"].IsScalar()) {
                    cerr << "Error: rom at index " << romindex << " is missing a name value.\n";
                    char temp[50];
                    sprintf(temp, "at index %d", romindex);
                    romName = temp;
                    isValid = false;
                } else {
                    romName = node["name"].Scalar();
                }
                if (!node["file"].IsDefined() || !node["file"].IsScalar()) {
                    cerr << "Error: rom " << romName << " is missing a file value.\n";
                    isValid = false;
                }
                if (node["header"].IsDefined() && (!node["header"].IsScalar() || (
                node["header"].Scalar() != "auto" && node["header"].Scalar() != "true" && node["header"].Scalar() != "false"))) {
                    cerr << "Error: rom " << romName << " does not have a valid header option - must be \"true\", \"false\", \"auto\", or not defined.\n";
                    isValid = false;
                }
            }
        }
    }
    return isValid;
}
