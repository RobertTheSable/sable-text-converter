#include "localecheck.h"
#include "unicode/locid.h"
#include <boost/locale.hpp>
#include <cstring>
#include <optional>
#include <stdexcept>

// Boost doesn't have a good way to validate user-provided locales easily
// Neither does ICU, but it has a less bad way.
bool isLocaleValid(const char *localeName)
{
    auto locale = icu::Locale::createCanonical(localeName);
    return (strlen(locale.getISO3Language()) != 0);
}

std::string normalize(const std::locale& locale, const std::string &in)
{
    return boost::locale::normalize(
        boost::locale::normalize(in, boost::locale::norm_nfd, locale),
        boost::locale::norm_nfc,
        locale
    );
}
namespace {
    std::optional<std::locale> lc = std::nullopt;
}

std::locale getLocale(const std::string &locale)
{
    if (!lc) {
        if (locale == "") {
            throw std::runtime_error("Locale was not set before trying to retieve it.");
        }
        lc = boost::locale::generator().generate(locale);
    }
    return *lc;
}
