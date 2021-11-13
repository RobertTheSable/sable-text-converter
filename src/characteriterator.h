#ifndef CHARACTERITERATOR_H
#define CHARACTERITERATOR_H
#include <vector>

namespace sable {
class CharacterIterator
{
    std::vector<int>::const_iterator m_Current;
    std::vector<int>::const_iterator m_End;
    int m_CharEncoding;
    int m_TotalWidth;;

public:
    CharacterIterator()=delete;
    CharacterIterator(const std::vector<int>::const_iterator& begin, const std::vector<int>::const_iterator& end, int width);
    bool haveNext() const;
    operator bool() const;
    CharacterIterator operator++(int);
    int operator*() const;
    int getWidth() const;
};
};

#endif // CHARACTERITERATOR_H
