#include "options.h"
#include <iostream>

using namespace std;

bool validateConfig(const YAML::Node& configYML) {
    bool isValid = true;
    if (!configYML["files"].IsDefined() || !configYML["files"].IsMap()) {
        isValid = false;
        cerr << "Error: files section is missing or not a map." << endl;
    } else {
        string mainDir = "";
        if (configYML["files"]["mainDir"].IsDefined() && configYML["files"]["mainDir"].IsScalar()) {
            mainDir = configYML["files"]["mainDir"].Scalar();
        }
        if (!configYML["files"]["input"].IsDefined() || !configYML["files"]["input"].IsMap()) {
            isValid = false;
            cerr << "Error: input section is missing or not a map." << endl;
        } else {
            if (!configYML["files"]["input"]["directory"].IsDefined() || !configYML["files"]["input"]["directory"].IsScalar()) {
                isValid = false;
                cerr << "Error: input directory for project is missing from files config." << endl;
            } else {
                fs::path textPath = fs::path(mainDir) / configYML["files"]["input"]["directory"].Scalar();
                if (!fs::exists(textPath)) {
                    isValid = false;
                    cerr << "Error: input directory " << textPath << " does not exist." << endl;
                }
            }
        }
        if (!configYML["files"]["output"].IsDefined() || !configYML["files"]["output"].IsMap()) {
            isValid = false;
            cerr << "Error: input section is missing or not a map." << endl;
        } else {
            if (!configYML["files"]["output"]["directory"].IsDefined() || !configYML["files"]["output"]["directory"].IsScalar()) {
                isValid = false;
                cerr << "Error: output directory for project is missing from files config." << endl;
            }
            if (!configYML["files"]["output"]["binaries"].IsDefined() || !configYML["files"]["output"]["binaries"].IsMap()) {
                isValid = false;
                cerr << "Error: output binaries subdirectory section is missing or not a map." << endl;
            } else {
                if (!configYML["files"]["output"]["binaries"]["mainDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["mainDir"].IsScalar()) {
                    isValid = false;
                    cerr << "Error: output binaries subdirectory mainDir value is missing from files config." << endl;
                }
                if (!configYML["files"]["output"]["binaries"]["textDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["textDir"].IsScalar()) {
                    isValid = false;
                    cerr << "Error: output binaries subdirectory textDir value is missing from files config." << endl;
                }
                if (configYML["files"]["output"]["binaries"]["extras"].IsDefined() && !configYML["files"]["output"]["binaries"]["extras"].IsSequence()) {
                    isValid = false;
                    cerr << "Error: extras section for output binaries must be a sequence." << endl;
                }
            }
            if (configYML["files"]["output"]["includes"].IsDefined() && !configYML["files"]["output"]["includes"].IsSequence()) {
                isValid = false;
                cerr << "Error: includes section for output must be a sequence." << endl;
            }
        }
        if (!configYML["files"]["romDir"].IsDefined() || !configYML["files"]["romDir"].IsScalar()) {
            isValid = false;
            cerr << "Error: romDir for project is missing from files config." << endl;
        } else {
            if (!fs::exists(configYML["files"]["romDir"].Scalar())) {
                isValid = false;
                cerr << "Error: directory \"" << configYML["files"]["romDir"].Scalar() << "\" does not exist." << endl;
            }
        }
    }
    if (!configYML["config"].IsDefined() || !configYML["config"].IsMap()) {
        isValid = false;
        cerr << "Error: config section is missing or not a map." << endl;
    } else {
        if (!configYML["config"]["directory"].IsDefined() || !configYML["config"]["directory"].IsScalar()) {
            isValid = false;
            cerr << "Error: directory for config section is missing or is not a scalar." << endl;
        }
        if (!configYML["config"]["inMapping"].IsDefined() || !configYML["config"]["inMapping"].IsScalar()) {
            isValid = false;
            cerr << "Error: inMapping for config section is missing or is not a scalar." << endl;
        }
        if (configYML["config"]["mapper"].IsDefined() && !configYML["config"]["mapper"].IsScalar()) {;
            isValid = false;
            cerr << "Error: config > mapper must be a string." << endl;
        }
    }
    if (!configYML["roms"].IsDefined()) {
        isValid = false;
        cerr << "Error: roms section is missing." << endl;
    } else {
        if (!configYML["roms"].IsSequence()) {
            isValid = false;
            cerr << "Error: roms section in config must be a sequence." << endl;
        } else {
            int romindex = 0;
            for(auto node: configYML["roms"]) {
                string romName;
                if (!node["name"].IsDefined() || !node["name"].IsScalar()) {
                    cerr << "Error: rom at index " << romindex << " is missing a name value." << endl;
                    char temp[50];
                    sprintf(temp, "at index %d", romindex);
                    romName = temp;
                    isValid = false;
                } else {
                    romName = node["name"].Scalar();
                }
                if (!node["file"].IsDefined() || !node["file"].IsScalar()) {
                    cerr << "Error: rom " << romName << " is missing a file value." << endl;
                    isValid = false;
                }
                if (node["header"].IsDefined() && (!node["header"].IsScalar() || (
                node["header"].Scalar() != "auto" && node["header"].Scalar() != "true" && node["header"].Scalar() != "false"))) {
                    cerr << "Error: rom " << romName << " does not have a valid header option - must be \"true\", \"false\", \"auto\", or not defined." << endl;
                    isValid = false;
                }
            }
        }
    }
    return isValid;
}

int parseOptions(int argc, char * argv[]) {
    int returnValue = ParseOptions::assembleRoms | ParseOptions::convertScript;
    if (argc > 0) {
        argv++;
        for (int i = 0; i < argc; i++) {
            string option = argv[i];
            if (option == "--help") {
                returnValue = ParseOptions::printHelp;
            } else if (option == "--no-assembly") {
                returnValue &= ~ParseOptions::assembleRoms;
            } else if (option == "--no-script") {
                returnValue &= ~ParseOptions::convertScript;
            } else if (option == "--verbose") {
                returnValue |= ParseOptions::verbose;
                returnValue &= ~ParseOptions::quiet;
            } else if (option == "--quiet") {
                returnValue |= ParseOptions::quiet;
                returnValue &= ~ParseOptions::verbose;
            }
        }
    }
    return returnValue;
}
