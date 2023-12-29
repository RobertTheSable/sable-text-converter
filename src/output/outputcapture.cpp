#include "outputcapture.h"

#include <stdexcept>
#ifndef _WIN32
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#endif

#ifndef _WIN32
OutputCapture::OutputCapture(std::ostream& sink)
    : OutputCapture(sink, O_NONBLOCK)
{

}

OutputCapture::OutputCapture(std::ostream &sink, int flags)
    : out{sink}, buffer{0}, saved_stdout{dup(STDOUT_FILENO)}
{
    if(pipe2(out_pipe, flags) != 0 ) {
        auto errnoS = std::string{std::strerror(errno)};
        throw std::runtime_error("failed to capture output: " + errnoS);
    }
    dup2(out_pipe[1], STDOUT_FILENO);
    close(out_pipe[1]);
}
#else
OutputCapture::OutputCapture()=default;
#endif

#ifndef _WIN32
OutputCapture::~OutputCapture()
{
    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);

    while(read(out_pipe[0], buffer, 101) > 0) {
        if (needWrite) {
            out << buffer;
        }
    }
    if (needWrite) {
        out << std::flush;
    }
}
#else
OutputCapture::~OutputCapture()=default;
#endif

void OutputCapture::write()
{
    needWrite = true;
}

