#include <catch2/catch.hpp>

#include "output/outputcapture.h"

#include <sstream>

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
TEST_CASE("Capturing puts")
{
    std::ostringstream sink;
    SECTION("Write not done")
    {
        {
            OutputCapture o{sink};
            std::puts("hello!");
        }
        REQUIRE(sink.str() == "");
    }
    SECTION("Write done")
    {
        {
            OutputCapture o{sink};
            std::puts("hello!");
            o.write();
        }
        REQUIRE(sink.str() == "hello!\n");
    }

    SECTION("capture fails")
    {
        REQUIRE_THROWS(OutputCapture{sink, O_APPEND});
    }
}
#else
#include <windows.h>

TEST_CASE("Getting the last error")
{
    std::ostringstream sink;
    {
        OutputCapture o{sink};
        LoadLibraryA("librarywhichdoesnotexist.dll");
    }
    REQUIRE(sink.str() != "");
}
#endif
