#include <iostream>
#include <cxxopts.hpp>
#include "cache.h"
#include "project.h"

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
    SableCache cache;
    unsigned int maxAddress = cache.getMaxAddress();
    if (showHelp) {
         cout << programOptions.help({"", "Group"}) << '\n';
    } else {
        sable::Project parser(starting_path.string());
        if (parser) {
            if (!options.count("a")) {
                parser.parseText();
            }
            if (!options.count("s")) {
                parser.writePatchData();
            }
        }
        cout << "Press enter to continue." << std::flush;
        cin.get();
        cout << std::endl;
    }
    return 0;
}
