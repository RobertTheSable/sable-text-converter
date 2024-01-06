#include "formatter.h"

namespace sable {

namespace formatter {

    std::string generateDefine(const std::string& label)
    {
        return std::string("!") + label;
    }

    std::string generateInclude(const fs::path &file, const fs::path &basePath, bool isBin)
    {

        std::string includePath = isBin ? "incbin " :"incsrc ";

        if (basePath.empty() || file.string().find_first_of(basePath.string()) != 0) {
            includePath += file.generic_string();
        } else {
            // can't use fs::relative because g++ 7 doesn't support it.
    #ifdef NO_FS_RELATIVE
            includePath +=  file.generic_string().substr(basePath.generic_string().length() + 1);
    #else
            includePath +=  fs::relative(file, basePath).generic_string();
    #endif
        }
        return includePath;
    }
} //formatter

} //sable


