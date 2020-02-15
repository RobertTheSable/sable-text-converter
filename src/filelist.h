#ifndef FILELIST_H
#define FILELIST_H
#include <queue>
#include <string>

typedef int Table;

class FileGroup {
private:
    std::queue<std::string> files;
    bool isTable;
    unsigned int tableLocation;
};

class FileList
{
public:
    FileList(std::string path);
    std::string getNextFile();
private:
    std::queue<std::string> files;
    std::queue<Table> tables;
};

#endif // FILELIST_H
