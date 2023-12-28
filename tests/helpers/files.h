#pragma once
#include "wrapper/filesystem.h"
#include <vector>
#include <fstream>

struct caseFile {
    fs::path name;
    std::string contents;
};

struct caseFileList
{
    fs::path folder;
    std::vector<fs::path> files;
    caseFileList(fs::path folder_): folder{folder_}
    {
        fs::create_directory(folder);
    }
    ~caseFileList()
    {
        fs::remove_all(folder);
    }

    void add(caseFile cf)
    {
        std::ofstream of((folder / cf.name).string());

        of << cf.contents;
        of.close();
        files.push_back(folder / cf.name);
    }

    void add(std::string name)
    {
        fs::create_directory(folder / name);
    }

    template<typename ...Args>
    void create(Args&& ...cases)
    {
        (add(cases), ...);
    }

};
