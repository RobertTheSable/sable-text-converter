#include <iostream>
#include <fstream>
#include <cxxopts.hpp>
#include "asar/asardll.h"
#include "script.h"

using namespace std;

bool validateConfig(const YAML::Node& configYML) {
    bool isValid = true;
    if (!configYML["files"].IsDefined() || !configYML["files"].IsMap()) {
        isValid = false;
        cout << "Error: files section is missing or not a map.\n";
    } else {
        string mainDir = "";
        if (configYML["files"]["mainDir"].IsDefined() && configYML["files"]["mainDir"].IsScalar()) {
            mainDir = configYML["files"]["mainDir"].Scalar();
        }
        if (!configYML["files"]["input"].IsDefined() || !configYML["files"]["input"].IsMap()) {
            isValid = false;
            cout << "Error: input section is missing or not a map.\n";
        } else {
            if (!configYML["files"]["input"]["directory"].IsDefined() || !configYML["files"]["input"]["directory"].IsScalar()) {
                isValid = false;
                cout << "Error: input directory for project is missing from files config.\n";
            } else {
                fs::path textPath = fs::path(mainDir) / configYML["files"]["input"]["directory"].Scalar();
                if (!fs::exists(textPath)) {
                    isValid = false;
                    cout << "Error: input directory " << textPath << " does not exist.\n";
                }
            }
        }
        if (!configYML["files"]["output"].IsDefined() || !configYML["files"]["output"].IsMap()) {
            isValid = false;
            cout << "Error: input section is missing or not a map.\n";
        } else {
            if (!configYML["files"]["output"]["directory"].IsDefined() || !configYML["files"]["output"]["directory"].IsScalar()) {
                isValid = false;
                cout << "Error: output directory for project is missing from files config.\n";
            }
            if (!configYML["files"]["output"]["binaries"].IsDefined() || !configYML["files"]["output"]["binaries"].IsMap()) {
                isValid = false;
                cout << "Error: output binaries subdirectory section is missing or not a map.\n";
            } else {
                if (!configYML["files"]["output"]["binaries"]["mainDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["mainDir"].IsScalar()) {
                    isValid = false;
                    cout << "Error: output binaries subdirectory mainDir value is missing from files config.\n";
                }
                if (!configYML["files"]["output"]["binaries"]["textDir"].IsDefined() || !configYML["files"]["output"]["binaries"]["textDir"].IsScalar()) {
                    isValid = false;
                    cout << "Error: output binaries subdirectory textDir value is missing from files config.\n";
                }
                if (configYML["files"]["output"]["binaries"]["extras"].IsDefined() && !configYML["files"]["output"]["binaries"]["extras"].IsSequence()) {
                    isValid = false;
                    cout << "Error: extras section for output binaries must be a sequence.\n";
                }
            }
            if (configYML["files"]["output"]["includes"].IsDefined() && !configYML["files"]["output"]["includes"].IsSequence()) {
                isValid = false;
                cout << "Error: includes section for output must be a sequence.\n";
            }
        }
        if (!configYML["files"]["romDir"].IsDefined() || !configYML["files"]["romDir"].IsScalar()) {
            isValid = false;
            cout << "Error: romDir for project is missing from files config.\n";
        } else {
            if (!fs::exists(configYML["files"]["romDir"].Scalar())) {
                isValid = false;
                cout << "Error: directory \"" << configYML["files"]["romDir"].Scalar() << "\" does not exist.\n";
            }
        }
    }
    if (!configYML["config"].IsDefined() || !configYML["config"].IsMap()) {
        isValid = false;
        cout << "Error: config section is missing or not a map.\n";
    } else {
        if (!configYML["config"]["directory"].IsDefined() || !configYML["config"]["directory"].IsScalar()) {
            isValid = false;
            cout << "Error: directory for config section is missing or is not a scalar.\n";
        }
        if (!configYML["config"]["inMapping"].IsDefined() || !configYML["config"]["inMapping"].IsScalar()) {
            isValid = false;
            cout << "Error: inMapping for config section is missing or is not a scalar.\n";
        }
        if (configYML["config"]["asarOptions"].IsDefined() &&
                !(configYML["config"]["asarOptions"].IsScalar() || configYML["config"]["asarOptions"].IsSequence()) ) {
            isValid = false;
            cout << "Error: asarOptions should be either a single string or a sequence.\n";
        }
    }
    if (!configYML["roms"].IsDefined()) {
        isValid = false;
        cout << "Error: roms section is missing.\n";
    } else {
        if (!configYML["roms"].IsSequence()) {
            isValid = false;
            cout << "Error: roms section in config must be a sequence.\n";
        } else {
            int romindex = 0;
            for(auto node: configYML["roms"]) {
                string romName;
                if (!node["name"].IsDefined() || !node["name"].IsScalar()) {
                    cout << "Error: rom at index " << romindex << " is missing a name value.\n";
                    char temp[50];
                    sprintf(temp, "at index %d", romindex);
                    romName = temp;
                    isValid = false;
                } else {
                    romName = node["name"].Scalar();
                }
                if (!node["file"].IsDefined() || !node["file"].IsScalar()) {
                    cout << "Error: rom " << romName << " is missing a file value.\n";
                    isValid = false;
                }
                if (node["header"].IsDefined() && (!node["header"].IsScalar() || (
                node["header"].Scalar() != "auto" && node["header"].Scalar() != "true" && node["header"].Scalar() != "false"))) {
                    cout << "Error: rom " << romName << " does not have a valid header option - must be \"true\", \"false\", \"auto\", or not defined.\n";
                    isValid = false;
                }
            }
        }
    }
    return isValid;
}

int main(int argc, char * argv[])
{
    cxxopts::Options programOptions(
                argv[0],
            "Program to convert scripts into Asar-compatible files for assembly and assemble files into a ROM.");
    programOptions.add_options()
            ("s,no-assembly", "Run without running Asar assembly.")
            ("a,no-script", "Run without updating the script.")
            ("p,project", "Project directory - defaults to working directory.", cxxopts::value<string>(), "DIR")
            ("v,verbose", "Run with increased verbosity.")
            ("q,quiet", "Run with reduced verbosity.")
            ("h,help", "Show this message.");
    auto options = programOptions.parse(argc, argv);
    bool showHelp = options.count("h") > 0;
    if (options.count("s") > 0 && options.count("a") > 0) {
        cout << "Nothing to do, printing help.\n";
        showHelp = true;
    }
    int verbosity = 1;
    if(options.count("v") > 0) {
        verbosity++;
    } else if (options.count("q") > 0) {
        verbosity--;
    }
    bool isCurrentDirNotProject = options.count("project") > 0;
    fs::path starting_path = isCurrentDirNotProject ? options["project"].as<string>() : fs::current_path().string();
    if (isCurrentDirNotProject) {
        fs::current_path(starting_path);
    }
    cout << fs::current_path().string() << '\n';
    if (showHelp) {
         cout << programOptions.help({"", "Group"}) << '\n';
    } else {
        bool parseScript = options.count("a") == 0;
        if (!fs::exists("config.yml")) {
            cerr << "Error: Missing config file\n";
        } else {
            YAML::Node configYML = YAML::LoadFile("config.yml");
            YAML::Node outputSection = configYML["files"]["output"];
            if (!validateConfig(configYML)) {
                cerr << "Config file contains errors, aborting.\n";
            }
            else {
                bool scriptWasBuilt = false;
                bool fontWasBuilt = false;
                fs::path fontLocation;
                if (parseScript) {
                    if (verbosity > 0) {
                        cout << "Converting script...\n";
                    }
                    fs::path configDir = configYML["config"]["directory"].Scalar();
                    Script sc((configDir / configYML["config"]["inMapping"].Scalar()).string().c_str());
                    fs::current_path(configYML["files"]["mainDir"].Scalar());
                    fs::path inputDirectory = configYML["files"]["input"]["directory"].Scalar();
                    std::string defaultMode = "normal";
                    if (configYML["config"]["defaultMode"].IsDefined() && configYML["config"]["defaultMode"].IsScalar()) {
                        defaultMode = configYML["config"]["defaultMode"].Scalar();
                    }
                    sc.loadScript(inputDirectory.string().c_str(), defaultMode, verbosity);
                    if (sc) {
                        if (verbosity > 0) {
                            cout << "Script converted, now generating files for assembly.\n";
                        }
                        scriptWasBuilt = sc.writeScript(outputSection);
                        if (outputSection["binaries"]["fonts"].IsDefined()) {
                            std::string baseOutputDir = outputSection["directory"].Scalar()
                                    + sable_preferred_separator
                                    + outputSection["binaries"]["mainDir"].Scalar()
                                    + sable_preferred_separator;
                            try {
                                std::string fontName = outputSection["binaries"]["fonts"]["dir"].as<std::string>();
                                fontLocation = baseOutputDir + fontName + sable_preferred_separator + fontName + ".asm";
                                fontWasBuilt = sc.writeFontData(fontLocation, outputSection["binaries"]["fonts"]["includes"]);
                            } catch (YAML::BadConversion &e) {
                                if (outputSection["binaries"]["fonts"].IsScalar()) {
                                    std::string fontName = outputSection["binaries"]["fonts"].as<std::string>();
                                    fontLocation = baseOutputDir + fontName + sable_preferred_separator + fontName + ".asm";
                                    fontWasBuilt = sc.writeFontData(fontLocation, outputSection["binaries"]["fonts"]["includes"]);
                                }
                            }
                        }
                    }
                    fs::current_path(starting_path);
                }
                if (options.count("s") == 0 && scriptWasBuilt) {
                    // Fix issue with symlinked directories.
                    fs::current_path(fs::path(argv[0]).parent_path());
                    if (asar_init()) {
                        fs::current_path(starting_path);
                        if (verbosity > 0) {
                            cout << "Asar initialized successfully, now assembling ROMs.\n";
                        }
                        for (auto romNode : configYML["roms"]) {
                            fs::path romPath = fs::path(configYML["files"]["romDir"].Scalar()) / romNode["file"].Scalar();
                            if (fs::exists(romPath)) {
                                std::string test = romPath.string();
                                ifstream romFile(romPath.string(), ios::in | ios::binary);
                                romFile.seekg(0, std::ios::end);
                                int outputSize = romFile.tellg();
                                char* outBuffer = new char[4194816];
                                for (int it = 0; it < 4194816 ; it++){
                                    outBuffer[it] = 0;
                                }
                                romFile.seekg(0, ios_base::beg);
                                romFile.read(outBuffer, outputSize);
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
                                    mainfile << "lorom\n"
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
                                                << include.Scalar() << ".asm\n";
                                    }
                                    if (fontWasBuilt) {
                                        mainfile << "incsrc " << fontLocation.string() << endl;
                                    }
                                    mainfile << "incsrc " << outputSection["directory"].Scalar() << "/" << "textDefines.exp\n";
                                    mainfile << "incsrc " << outputSection["directory"].Scalar() << "/" << "text.asm\n";
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
                                    if (verbosity > 0) {
                                        cout << "Assembly for " << romNode["name"].Scalar() << " completed successfully.\n";
                                    }
                                } else {
                                    fs::current_path(starting_path);
                                    int errorCount;
                                    const errordata* errors = asar_geterrors(&errorCount);
                                    for(int i = 0; i < errorCount; i++) {
                                        cerr << errors[i].fullerrdata << endl;
                                    }
                                }
                                delete [] outBuffer;
                                asar_reset();
                            } else {
                                cerr << "Error: Rom file " << fs::absolute(romPath) << " is missing.\n";
                            }
                        }
                    } else {
                        cerr << "Could not initialize Asar.\n";
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
