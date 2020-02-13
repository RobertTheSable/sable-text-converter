#include <iostream>
#include <fstream>
#include <cxxopts.hpp>
#include "options.h"
#include "asar/asardll.h"
#include "script.h"
#include "cache.h"

using namespace std;

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
    SableCache cache;
    unsigned int maxAddress = cache.getMaxAddress();
    if (showHelp) {
         cout << programOptions.help({"", "Group"}) << '\n';
    } else {
        bool parseScript = options.count("a") == 0;
        if (!fs::exists("config.yml")) {
            cerr << "Error: Missing config file\n";
        } else {
            YAML::Node configYML = YAML::LoadFile("config.yml");
            YAML::Node outputSection = configYML["files"]["output"];
            std::string mapType = configYML["config"]["mapper"].IsDefined() ?
                configYML["config"]["mapper"].as<string>() :
                "lorom";
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
                    Script sc(
                            (configDir / configYML["config"]["inMapping"].Scalar()).string().c_str(),
                            mapType.c_str()
                            );
                    if (!sc) {
                        cerr << "Error: mapper " << mapType << " is not supported." <<  endl;
                    } else {
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
                            maxAddress = sc.getMaxAddress();
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
                    }
                } else {
                    scriptWasBuilt = true;
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
                                int inputSize = romFile.tellg();
                                int outputSize;
                                if (!parseScript) {
                                    if (mapType == "exlorom") {
                                        maxAddress = EXLoROMToPC(0x018000);
                                    }
                                }
                                if (maxAddress < inputSize) {
                                    if (maxAddress == 0) {
                                        cerr << "Warning: output size could not be determined normally.\n"
                                             << "Either the script wasn't parse or cache couldn't be read.\n";
                                    }
                                    outputSize = inputSize;
                                } else {
                                    if (maxAddress >= NORMAL_ROM_MAX_SIZE) {
                                        if (maxAddress >= 6291456) {
                                            outputSize = 8388608;
                                        } else {
                                            outputSize = 6291456;
                                        }
                                    } else if (maxAddress < 131072) {
                                        outputSize = 131072;
                                    } else if (maxAddress < 262144) {
                                        outputSize = 262144;
                                    } else {
                                        outputSize = (1 + ((maxAddress - 1) / 524288)) * 524288;
                                    }
                                }
                                char* outBuffer = new char[outputSize + 512]();
                                romFile.seekg(0, ios_base::beg);
                                romFile.read(outBuffer, inputSize);
                                romFile.close();
                                int headerSize;
                                if(romNode["header"].IsDefined() && romNode["header"].Scalar() != "auto") {
                                    if (romNode["header"].Scalar() == "true") {
                                        headerSize = 512;
                                    } else if (romNode["header"].Scalar() == "false") {
                                        headerSize = 0;
                                    }
                                } else {
                                    headerSize = ((inputSize % 1024));
                                }
                                if ((headerSize%512)== 0) {
                                    char* extBuffer = &outBuffer[headerSize+NORMAL_ROM_MAX_SIZE];
                                    if (outputSize > NORMAL_ROM_MAX_SIZE) {
                                        if (mapType == "exlorom") {
                                            int romTypeLocation = LoROMToPC(HEADER_LOCATION,(headerSize != 0)) + 0x17;
                                            int oldRomType = outBuffer[romTypeLocation];
                                            outBuffer[romTypeLocation] = oldRomType | 0x01;
                                            if ((inputSize - headerSize) <= NORMAL_ROM_MAX_SIZE) {
                                                for (int i = 0; i < 0x8000; i++) {
                                                    outBuffer[headerSize+NORMAL_ROM_MAX_SIZE+ i] = outBuffer[headerSize + i];
                                                }
                                            }
                                        }
                                    }
                                    fs::current_path(configYML["files"]["mainDir"].Scalar());
                                    fs::path patchFile = romNode["name"].Scalar() +".asm";
                                    bool regenerate = !romNode["regenMain"].IsDefined() || (romNode["regenMain"].IsScalar() && romNode["regenMain"].Scalar() != "no");
                                    if ((!fs::exists(patchFile) || regenerate) && parseScript){
                                        ofstream mainfile(patchFile.string());
                                        mainfile << mapType << '\n'
                                                << '\n';
                                        for (auto include : romNode["includes"]) {
                                            mainfile << "incsrc " << outputSection["directory"].Scalar() << "/"
                                                    << include.Scalar() << '\n';
                                        }
                                        for (auto include : outputSection["includes"]) {
                                            mainfile << "incsrc " << outputSection["directory"].Scalar() << "/"
                                                    << include.Scalar() << '\n';
                                        }
                                        for (auto include : outputSection["binaries"]["extras"]) {
                                            mainfile << "incsrc " << outputSection["directory"].Scalar() << "/"
                                                    << outputSection["binaries"]["mainDir"] << "/"
                                                    << include.Scalar() << "/"
                                                    << include.Scalar() << ".asm\n";
                                        }
                                        if (fontWasBuilt) {
                                            mainfile << "incsrc " << fontLocation.string() << '\n';
                                        }
                                        mainfile << "incsrc " << outputSection["directory"].Scalar() << "/" << "textDefines.exp\n";
                                        mainfile << "incsrc " << outputSection["directory"].Scalar() << "/" << "text.asm\n";
                                        mainfile.close();
                                    }
                                    if (asar_patch(
                                            patchFile.string().c_str(),
                                            outBuffer+headerSize,
                                            outputSize,
                                            &outputSize
                                        )) {
                                        fs::current_path(starting_path);
                                        string outPath = (fs::path(configYML["files"]["romDir"].Scalar())
                                                / romNode["name"].Scalar()).string()
                                                + ((headerSize == 512) ? ".smc" : ".sfc");
                                        ofstream output(outPath, ios::binary);
                                        if (headerSize == 512) {
                                            output.write(outBuffer, 512 + outputSize);
                                        } else {
                                            output.write(outBuffer, outputSize);
                                        }
                                        output.close();
                                        int printcount;
                                        const char* const* prints = asar_getprints(&printcount);
                                        for (int i = 0; i< printcount; i++)  {
                                            cout << prints[i] << '\n';
                                        }
                                        if (verbosity > 0) {
                                            cout << "Assembly for " << romNode["name"].Scalar() << " completed successfully.\n";
                                        }
                                    } else {
                                        fs::current_path(starting_path);
                                        int errorCount;
                                        const errordata* errors = asar_geterrors(&errorCount);
                                        for(int i = 0; i < errorCount; i++) {
                                            cerr << errors[i].fullerrdata << '\n';
                                        }
                                    }
                                    delete [] outBuffer;
                                    asar_reset();
                                } else {
                                    cerr << "Error: Rom file " << fs::absolute(romPath) << " has a malformed header.\n";
                                }
                            } else {
                                cerr << "Error: Rom file " << fs::absolute(romPath) << " is missing.\n";
                            }
                        }
                    } else {
                        cerr << "Could not initialize Asar.\n";
                    }
                } else {
                    cerr << "Could not start assembly.\n";
                }
            }
        }
        cout << "Press enter to continue.";
        cin.get();
        cout << '\n';
    }
    return 0;
}
