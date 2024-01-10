#ifndef LOCALECHECK_H
#define LOCALECHECK_H

#include <string>

namespace sable {

bool isLocaleValid(const char* localeName);
std::string normalize(const std::locale& locale, const std::string& in);
std::locale getLocale(const std::string &locale);

}

#endif // LOCALECHECK_H
