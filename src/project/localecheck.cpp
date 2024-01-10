#include "localecheck.h"
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <cstring>

namespace sable {

bool isLocaleValid(const char *localeName)
{
    auto locale = icu::Locale::createCanonical(localeName);
    return (strlen(locale.getISO3Language()) != 0);
}

}
