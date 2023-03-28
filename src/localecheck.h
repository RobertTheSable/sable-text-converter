#ifndef LOCALECHECK_H
#define LOCALECHECK_H

#include <string>
#include <locale>

bool isLocaleValid(const char* localeName);
std::locale getLocale(const std::string &locale);
std::string normalize(const std::locale& locale, const std::string& in);

#endif // LOCALECHECK_H
