#ifndef OUTPUTCAPTURE_H
#define OUTPUTCAPTURE_H

#include <iostream>

// Asar will output an error if it fails to load its DLL from any one of the paths
// This class captures that output and outputs only if all paths failed.
class OutputCapture
{
    std::ostream &out;
#ifndef _WIN32
    char buffer[101];
    int saved_stdout;
    int out_pipe[2];
#endif
    bool needWrite = false;
public:
    OutputCapture(std::ostream& sink);
#ifndef _WIN32
    OutputCapture(std::ostream& sink, int flags);
#endif
    ~OutputCapture();
    void write();
    void flush();
};

#endif // OUTPUTCAPTURE_H
