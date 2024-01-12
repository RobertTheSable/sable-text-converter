#include <catch2/catch.hpp>
#include "project/util.h"
#include "data/mapper.h"

TEST_CASE("File size calculation.")
{
    using sable::util::calculateFileSize, sable::util::getFileSizeString;
    SECTION("Calculated from config.")
    {
        REQUIRE(calculateFileSize("2048") == 2048);
        REQUIRE(calculateFileSize("2KB") == 2048);
        REQUIRE(calculateFileSize("2 kb ") == 2048);
        REQUIRE(calculateFileSize("2M") == 2097152);
        REQUIRE(calculateFileSize("2MB") == calculateFileSize("2mb"));
        REQUIRE(calculateFileSize("2MB") == calculateFileSize("2M"));
        REQUIRE(calculateFileSize("2KB") == calculateFileSize("2K"));
        REQUIRE(calculateFileSize("3.5MB") == 3670016);
        REQUIRE(calculateFileSize("2GB") == 0);
        REQUIRE(calculateFileSize("10mb") == 0);
        REQUIRE(calculateFileSize("2.5") == 0);
        REQUIRE(calculateFileSize("") == 0);
        REQUIRE(calculateFileSize("test") == 0);
        REQUIRE(calculateFileSize("2M test") == 0);
    }

    SECTION("File size string generation")
    {
        REQUIRE(getFileSizeString(128 * 1024) == "128kb");
        REQUIRE(getFileSizeString(2 * 1024 * 1024) == "2mb");
        REQUIRE(getFileSizeString(((2*1024) + 512) * 1024) == "2.5mb");
        REQUIRE(getFileSizeString(sable::util::ROM_MAX_SIZE) == "8mb");
        REQUIRE(getFileSizeString(0) == "");
    }
}
