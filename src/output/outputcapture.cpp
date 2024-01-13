#include "outputcapture.h"

#include <stdexcept>
#ifndef _WIN32
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#else
#include <windows.h>
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
OutputCapture::OutputCapture(std::ostream& sink): out{sink} {}

#endif

OutputCapture::~OutputCapture()
{
    flush();
}

void OutputCapture::write()
{
    needWrite = true;
}

void OutputCapture::flush()
{
#ifndef _WIN32
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
#else
    DWORD errCode = GetLastError();
    if (errCode == 0) {
        return;
    }
    LPSTR errMsg = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&errMsg,
        0,
        nullptr
    );

    if (needWrite) {
        out << std::string(errMsg);
        if (errCode == ERROR_MOD_NOT_FOUND) {
            out << "Make sure that asar.dll is in the same folder as sable.\n";
        }
        out << std::flush;
    }
    LocalFree(errMsg);
#endif
}

