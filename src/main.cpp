#include <iostream>
#include <cxxopts.hpp>
#include "cache.h"
#include "project.h"
#include "exceptions.h"

int main(int argc, char * argv[])
{
    using std::cout, std::cerr;
    cxxopts::Options programOptions(
                argv[0],
            "Program to convert scripts into Asar-compatible files for assembly and assemble files into a ROM.");
    programOptions.add_options()
            ("s,no-assembly", "Run without running Asar assembly.")
            ("a,no-script", "Run without updating the script.")
            ("p,project", "Project directory - defaults to working directory.", cxxopts::value<std::string>(), "DIR")
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
    fs::path starting_path = isCurrentDirNotProject ? options["project"].as<std::string>() : fs::current_path().string();
    if (showHelp) {
         cout << programOptions.help({"", "Group"}) << '\n';
    } else {
        try {
            sable::Cache cache(starting_path.string());
            unsigned int maxAddress = cache.getMaxAddress();
            sable::Project parser(starting_path.string());
            if (parser) {
                if (!options.count("a")) {
                    parser.parseText();
                    cache.setMaxAddress(parser.getMaxAddress());
                    cache.write();
                }
                if (!options.count("s")) {
                    parser.writePatchData();
                }
            }
        } catch (sable::FontError &e){
            cerr << "Error in input mapping file:\n"
                 << e.what() << std::endl;
        } catch (sable::ParseError &e) {
            cerr << "Fatal error during parsing:\n"
                      << e.what() << std::endl;
        } catch(sable::ASMError &e) {
            cerr << std::string("Output error:\n" )
                    + e.what() << std::endl;
        } catch (sable::ConfigError &e) {
            cerr << "Error(s) in project config: \n"
                 << e.what() << std::endl;
        }
        cout << "Press enter to continue." << std::flush;
        std::cin.get();
        cout << std::endl;
    }
    return 0;
}
