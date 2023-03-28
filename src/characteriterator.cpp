#include "characteriterator.h"

using sable::CharacterIterator;

CharacterIterator::CharacterIterator(const std::vector<int>::const_iterator &begin, const std::vector<int>::const_iterator &end, int width):
    m_Current(begin), m_End(end), m_CharEncoding(0), m_TotalWidth(width)
{
    if (m_Current != m_End) {
        m_CharEncoding = *m_Current;
    }
}

bool CharacterIterator::haveNext() const
{
    return m_Current != m_End;
}

sable::CharacterIterator CharacterIterator::operator++(int)
{
    CharacterIterator cItr = *this;
    if (haveNext()) {
        ++m_Current;
        if (!haveNext()) {
            m_CharEncoding = 0;
        } else {
            m_CharEncoding = *m_Current;
        }

    }
    return cItr;
}

int CharacterIterator::operator*() const
{
    return m_CharEncoding;
}

int CharacterIterator::getWidth() const
{
    return m_TotalWidth;
}

sable::CharacterIterator::operator bool() const
{
    return haveNext();
}
