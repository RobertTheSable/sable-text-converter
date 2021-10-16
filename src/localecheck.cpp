#include "localecheck.h"
#include "unicode/locid.h"
#include <cstring>

// Boost doesn't have a good way to validate user-provided locales easily
// Neither does ICU, but it has a less bad way.
bool isLocaleValid(const char *localeName)
{
    auto locale = icu::Locale::createCanonical(localeName);
    return (strlen(locale.getISO3Language()) != 0);
}
