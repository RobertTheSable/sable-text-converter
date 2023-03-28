#include "unicode.h"


BreakIterator::BreakIterator(bool word, const std::string &data, const icu_70::Locale &locale)
{
    // needs to be explicitly initialized as utf8 on MSVC
    _u16Data = icu::UnicodeString::fromUTF8(data);
    UErrorCode _err = U_ZERO_ERROR;
    if (word) {
        _itr.reset(icu::BreakIterator::createWordInstance(locale, _err));
    } else {
        _itr.reset(icu::BreakIterator::createCharacterInstance(locale, _err));
    }
    if (U_FAILURE(_err)) {
        throw BoundsException("Failed to create iterator");
    }
    _word = word;

    _itr->setText(_u16Data);
    _begin = _itr->first();
    _cur = _begin;
    _next = _itr->next();
}

BreakIterator &BreakIterator::operator=(BreakIterator rhs)
{
    _u16Data = rhs._u16Data;
    UErrorCode _err = U_ZERO_ERROR;
    _itr.reset(rhs._itr->clone());
    _begin = rhs._begin;
    _cur = rhs._cur;
    _next = rhs._next;
    return *this;
}

BreakIterator::BreakIterator(const BreakIterator &rhs) {
    _u16Data = rhs._u16Data;
    UErrorCode _err = U_ZERO_ERROR;
    _itr.reset(rhs._itr->clone());
    _begin = rhs._begin;
    _cur = rhs._cur;
    _next = rhs._next;
}

bool BreakIterator::done() {
    return _next == UBRK_DONE;
}

bool BreakIterator::atStart() {
    return _cur == _begin;
}

BreakIterator BreakIterator::operator++(int) {
    auto tmp = *this;
    this->operator++();
    return tmp;
}

BreakIterator &BreakIterator::operator--() {
    if (_cur != _begin) {
        if (_next == UBRK_DONE) {
            _next = _cur;
            _cur = _itr->previous();
            _itr->next();
        } else {
            _itr->previous();
            _cur = _itr->previous();
            _next= _itr->next();
        }
    }
    return *this;
}

std::string BreakIterator::operator*() {
    if (_next == UBRK_DONE) {
        return "";
    }
    auto tmp = _u16Data.tempSubStringBetween(_cur, _next);
    std::string tmpS;
    return tmp.toUTF8String(tmpS);
}

UChar32 BreakIterator::ufront() const {
    return _u16Data.char32At(_cur);
}

std::string BreakIterator::front() const {
    if (_next == UBRK_DONE) {
        return "";
    }
    auto tmp = _u16Data.tempSubStringBetween(_cur, _cur+1);
    std::string tmpS;
    return tmp.toUTF8String(tmpS);
}

BreakIterator &BreakIterator::operator++() {
    _cur = _next;
    if (_cur != UBRK_DONE) {
        _next = _itr->next();
    }
    return *this;
}
