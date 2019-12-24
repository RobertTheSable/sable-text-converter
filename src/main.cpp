#include <iostream>
#include <fstream>
#include "asar/asardll.h"
#include "script.h"

namespace ParseOptions {
    static const int nothingToDo = 0;
    static const int printHelp = 1;
    static const int assembleRoms = 2;
    static const int convertScript = 4;
    static const int verbose = 8;
}
using namespace std;

bool validateConfig(const YAML::Node& configYML) {
    bool isValid = true;
    if (!configYML["files"].IsDefined() || !configYML["files"].IsMap()) {
        isValid = false;
        cout << "Error: files section is missing or not a map." << endl;
    } else {
        string mainDir = "";
        if (configYML["files"]["mainDir"].IsDefined() && configYML["files"]["mainDir"].IsScalar()) {
            mainDir = configYML["files"]["mainDir"].Scalar();
        }
        if (!configYML["files"]["input"].IsDefined() || !configYML["files"]["input"].IsMap()) {
            isValid = false;
            cout << "Error: input section is missing or not a map." << endl;
        } else {
            if (!configYML["files"]["input"]["directory"].IsDefined() || !configYML["files"]["input"]["directory"].IsScalar()) {
                isValid = false;
                cout << "Error: input directory for project is missing from files config." << endl;
            } else {
                fs::path textPath = fs::path(mainDir) / configYML["files"]["input"]["directory"].Scalar();
                if (!fs::exists(textPath)) {
                    isValid = false;
                    cout << "Error: input directory " << textPath << " does not exist." << endl;
                }
            }
        }
        if (!configYML["files"]["output"].IsDefined() || !configYML["files"]["output"].IsMap()) {
            isValid = false;
            cout << "Error: input section is missing or not a map." << endl;
        } else {
            if (!configYML["files"]["output"]["directory"].IsDefined() || !configYML["files"]["output"]["directory"].IsScalar()) {
                isValid = false;
                cout << "Error: output directory for project is missing from files config." << endl;
            }
            if (!configYML["files"]["output"]["binaries"].IsDefined() || !configYML["files"]["output"]["binaries"].IsMap()) {
                isValid = false;
                cout << "Error: output binaries subdirectory section is missing or not a map." << endl;
            } else {
                if (!configYML["files"]["output"]["binaries"]["mainDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["mainDir"].IsScalar()) {
                    isValid = false;
                    cout << "Error: output binaries subdirectory mainDir value is missing from files config." << endl;
                }
                if (!configYML["files"]["output"]["binaries"]["textDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["textDir"].IsScalar()) {
                    isValid = false;
                    cout << "Error: output binaries subdirectory textDir value is missing from files config." << endl;
                }
                if (configYML["files"]["output"]["binaries"]["extras"].IsDefined() && !configYML["files"]["output"]["binaries"]["extras"].IsSequence()) {
                    isValid = false;
                    cout << "Error: extras section for output binaries must be a sequence." << endl;
                }
            }
            if (configYML["files"]["output"]["includes"].IsDefined() && !configYML["files"]["output"]["includes"].IsSequence()) {
                isValid = false;
                cout << "Error: includes section for output must be a sequence." << endl;
            }
        }
        if (!configYML["files"]["romDir"].IsDefined() || !configYML["files"]["romDir"].IsScalar()) {
            isValid = false;
            cout << "Error: romDir for project is missing from files config." << endl;
        } else {
            if (!fs::exists(configYML["files"]["romDir"].Scalar())) {
                isValid = false;
                cout << "Error: directory \"" << configYML["files"]["romDir"].Scalar() << "\" does not exist." << endl;
            }
        }
    }
    if (!configYML["config"].IsDefined() || !configYML["config"].IsMap()) {
        isValid = false;
        cout << "Error: config section is missing or not a map." << endl;
    } else {
        if (!configYML["config"]["directory"].IsDefined() || !configYML["config"]["directory"].IsScalar()) {
            isValid = false;
            cout << "Error: directory for config section is missing or is not a scalar." << endl;
        }
        if (!configYML["config"]["inMapping"].IsDefined() || !configYML["config"]["inMapping"].IsScalar()) {
            isValid = false;
            cout << "Error: inMapping for config section is missing or is not a scalar." << endl;
        }
        if (configYML["config"]["asarOptions"].IsDefined() &&
                !(configYML["config"]["asarOptions"].IsScalar() || configYML["config"]["asarOptions"].IsSequence()) ) {
            isValid = false;
            cout << "Error: asarOptions should be either a single string or a sequence." << endl;
        }
    }
    if (!configYML["roms"].IsDefined()) {
        isValid = false;
        cout << "Error: roms section is missing." << endl;
    } else {
        if (!configYML["roms"].IsSequence()) {
            isValid = false;
            cout << "Error: roms section in config must be a sequence." << endl;
        } else {
            int romindex = 0;
            for(auto node: configYML["roms"]) {
                string romName;
                if (!node["name"].IsDefined() || !node["name"].IsScalar()) {
                    cout << "Error: rom at index " << romindex << " is missing a name value." << endl;
                    char temp[50];
                    sprintf(temp, "at index %d", romindex);
                    romName = temp;
                    isValid = false;
                } else {
                    romName = node["name"].Scalar();
                }
                if (!node["file"].IsDefined() || !node["file"].IsScalar()) {
                    cout << "Error: rom " << romName << " is missing a file value." << endl;
                    isValid = false;
                }
                if (node["header"].IsDefined() && (!node["header"].IsScalar() || (
                node["header"].Scalar() != "auto" && node["header"].Scalar() != "true" && node["header"].Scalar() != "false"))) {
                    cout << "Error: rom " << romName << " does not have a valid header option - must be \"true\", \"false\", \"auto\", or not defined." << endl;
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
            }
        }
    }
    return returnValue;
}

int main(int argc, char * argv[])
{
    fs::path&& starting_path = fs::current_path().string();
    int options = parseOptions(argc-1, argv);
    if (options == ParseOptions::nothingToDo) {
        cout << "Nothing to do, printing help." << endl;
        options = ParseOptions::printHelp;
    }
    int verbosity = 0;
    if((options&ParseOptions::verbose) != 0) {
        options &= ~ParseOptions::verbose;
        verbosity++;
    }
    if (options == ParseOptions::printHelp) {
        cout << "Usage: " << argv[0] << " <options>" << endl
             << "Program to convert scripts into Asar-compatible files for assembly" << endl
             << "and assemble files into a ROM." << endl
             << endl
             << "Options:" << endl
             << "--no-assembly : run without running Asar assembly." << endl
             << "--no-script : run without updating the script." << endl
             << "--verbose : run with increased verbosity." << endl;
    } else {
        bool parseScript = (options&ParseOptions::convertScript) != 0;
        if (!fs::exists("config.yml")) {
            cout << "Error: Missing config file" << endl;
        } else {
            YAML::Node configYML = YAML::LoadFile("config.yml");
            YAML::Node outputSection = configYML["files"]["output"];
            if (!validateConfig(configYML)) {
                cout << "Config file contains errors, aborting." << endl;
            }
            else {
                bool scriptWasBuilt = false;
                bool fontWasBuilt = false;
                fs::path fontLocation;
                if (parseScript) {
                    if (verbosity > 0) {
                        cout << "Converting script..." << endl;
                    }
                    fs::path&& configDir = configYML["config"]["directory"].Scalar();
                    Script sc((configDir / configYML["config"]["inMapping"].Scalar()).string().c_str());
                    fs::current_path(configYML["files"]["mainDir"].Scalar());
                    fs::path&& inputDirectory = configYML["files"]["input"]["directory"].Scalar();
                    std::string defaultMode = "normal";
                    if (configYML["config"]["defaultMode"].IsDefined() && configYML["config"]["defaultMode"].IsScalar()) {
                        defaultMode = configYML["config"]["defaultMode"].Scalar();
                    }
                    sc.loadScript(inputDirectory.string().c_str(), defaultMode, verbosity);
                    if (sc) {
                        scriptWasBuilt = sc.writeScript(outputSection);
                        if (outputSection["binaries"]["fonts"].IsDefined()) {
                            std::string baseOutputDir = outputSection["directory"].Scalar()
                                    + fs::path::preferred_separator
                                    + outputSection["binaries"]["mainDir"].Scalar()
                                    + fs::path::preferred_separator;
                            try {
                                std::string fontName = outputSection["binaries"]["fonts"]["dir"].as<std::string>();
                                fontLocation = baseOutputDir + fontName + fs::path::preferred_separator + fontName + ".asm";
                                fontWasBuilt = sc.writeFontData(fontLocation, outputSection["binaries"]["fonts"]["includes"]);
                            } catch (YAML::BadConversion &e) {
                                if (outputSection["binaries"]["fonts"].IsScalar()) {
                                    std::string fontName = outputSection["binaries"]["fonts"].as<std::string>();
                                    fontLocation = baseOutputDir + fontName + fs::path::preferred_separator + fontName + ".asm";
                                    fontWasBuilt = sc.writeFontData(fontLocation, outputSection["binaries"]["fonts"]["includes"]);
                                }
                            }
                        }
                    }
                    fs::current_path(starting_path);
                }
                if ((options&ParseOptions::assembleRoms) != 0 && scriptWasBuilt) {
                    if (verbosity > 0) {
                        cout << "Assembling ROMs..." << endl;
                    }
                    // Fix issue with symlinked directories.
                    fs::current_path(starting_path);
                    if (asar_init()) {
                        if (verbosity > 0) {
                            cout << "Asar initialized successfully." << endl;
                        }
                        for (auto romNode : configYML["roms"]) {
                            fs::path romPath = fs::path(configYML["files"]["romDir"].Scalar()) / romNode["file"].Scalar();
                            if (fs::exists(romPath)) {
                                std::string test = romPath.string();
                                ifstream romFile(romPath.string(), ios::in | ios::binary);
                                romFile.seekg(0, std::ios::end);
                                int outputSize = romFile.tellg();
                                char outBuffer[4194816];
                                for (int it = 0; it < 4194816 ; it++){
                                    outBuffer[it] = 0;
                                }
                                romFile.seekg(0, ios_base::beg);
                                romFile.read(reinterpret_cast<char*>(outBuffer), outputSize);
                                romFile.close();
                                int headerSize;
                                if(romNode["header"].IsDefined() && romNode["header"].Scalar() != "auto") {
                                    if (romNode["header"].Scalar() == "true") {
                                        headerSize = 512;
                                    } else if (romNode["header"].Scalar() == "false") {
                                        headerSize = 0;
                                    }
                                } else {
                                    headerSize = ((outputSize % 1024));
                                }
                                fs::current_path(configYML["files"]["mainDir"].Scalar());
                                fs::path patchFile = romNode["name"].Scalar() +".asm";
                                bool regenerate = !romNode["regenMain"].IsDefined() || (romNode["regenMain"].IsScalar() && romNode["regenMain"].Scalar() != "no");
                                if (!fs::exists(patchFile) || regenerate){
                                    ofstream mainfile(patchFile.string());
                                    mainfile << "lorom" << endl
                                            << endl;
                                    for (auto include : romNode["includes"]) {
                                        mainfile << "incsrc " << outputSection["directory"].Scalar() << "/"
                                                << include.Scalar() << endl;
                                    }
                                    for (auto include : outputSection["includes"]) {
                                        mainfile << "incsrc " << outputSection["directory"].Scalar() << "/"
                                                << include.Scalar() << endl;
                                    }
                                    for (auto include : outputSection["binaries"]["extras"]) {
                                        mainfile << "incsrc " << outputSection["directory"].Scalar() << "/"
                                                << outputSection["binaries"]["mainDir"] << "/"
                                                << include.Scalar() << "/"
                                                << include.Scalar() << ".asm" << endl;
                                    }
                                    if (fontWasBuilt) {
                                        mainfile << "incsrc " << fontLocation.string() << endl;
                                    }
                                    mainfile << "incsrc " << outputSection["directory"].Scalar() << "/" << "textDefines.exp" << endl;
                                    mainfile << "incsrc " << outputSection["directory"].Scalar() << "/" << "text.asm" << endl;
                                    mainfile.close();
                                }
                                if ((headerSize%512)== 0 && asar_patch(
                                            patchFile.string().c_str(),
                                            outBuffer+headerSize,
                                            4*1024*1024,
                                            &outputSize
                                            )) {
                                    fs::current_path(starting_path);
                                    string outPath = (fs::path(configYML["files"]["romDir"].Scalar())
                                            / romNode["name"].Scalar()).string()
                                            + ((headerSize == 512) ? ".smc" : ".sfc");
                                    ofstream output(outPath, ios::binary);
                                    if (headerSize == 512) {
                                        output.write(outBuffer, 512 + (4*1024*1024));
                                    } else {
                                        output.write(outBuffer, (4*1024*1024));
                                    }
                                    output.close();
                                    int printcount;
                                    const char* const* prints = asar_getprints(&printcount);
                                    for (int i = 0; i< printcount; i++)  {
                                        cout << prints[i] << endl;
                                    }
                                    cout << "Assembly for " << romNode["name"].Scalar() << " completed successfully." << endl;
                                } else {
                                    fs::current_path(starting_path);
                                    int errorCount;
                                    const errordata* errors = asar_geterrors(&errorCount);
                                    for(int i = 0; i < errorCount; i++) {
                                        cerr << errors[i].fullerrdata << endl;
                                    }
                                }
                                asar_reset();
                            } else {
                                cout << "Error: Rom file " << fs::absolute(romPath) << " is missing." << endl;
                            }
                        }
                    } else {
                        cerr << "Could not initialize Asar." << endl;
                    }
                }
            }
        }
        cout << "Press enter to continue.";
        cin.get();
        cout << endl;
    }
    return 0;
}
