#ifndef UNICODE_H
#define UNICODE_H

#include <memory>
#include <stdexcept>
#include <unicode/brkiter.h>
#include <unicode/locid.h>

struct BoundsException : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class BreakIterator
{
    icu::UnicodeString _u16Data;
    std::unique_ptr<icu::BreakIterator> _itr = nullptr;
    bool _word;
    int32_t _begin, _cur, _next;
public:
    BreakIterator(bool word, const std::string& data, const icu::Locale& locale);
    BreakIterator& operator=(BreakIterator rhs);
    BreakIterator(const BreakIterator& rhs);

    bool done();
    bool atStart();

    BreakIterator& operator++();
    BreakIterator operator++(int);
    BreakIterator& operator--();
    std::string operator*();

    UChar32 ufront() const;
    std::string front() const;
};

#endif // UNICODE_H
