#include "fontlist.h"

namespace sable {
FontList::FontList()
{

}
void FontList::AddFont(const std::string& name, Font&& font)
{
    if (m_List.find(name) == m_List.end()) {
        m_List.insert(std::make_pair(name, font));
    }
}

FontList::iterator FontList::begin() const
{
    return iterator(m_List.begin());
}

FontList::iterator FontList::end() const
{
    return iterator(m_List.end());
}

const Font &FontList::operator[](const std::string &name) const
{
    if (m_List.find(name) == m_List.end()) {
        throw std::out_of_range(name + " not found");
    }
    return m_List.at(name);
}

bool FontList::contains(const std::string &name) const
{
    return (m_List.find(name) != m_List.end());
}

size_t FontList::size() const
{
    return m_List.size();
}

FontList::iterator::iterator(Storage::const_iterator &&position)
{
    m_position = position;
}

FontList::iterator &FontList::iterator::operator++()
{
    ++m_position;
    return *this;
}

const Font *FontList::iterator::operator->() const
{
    return &m_position->second;
}

const Font &FontList::iterator::operator*() const
{
    return m_position->second;
}

std::string FontList::iterator::getName() const
{
    return m_position->first;
}

bool operator!=(const FontList::iterator& lsh, const FontList::iterator& rhs)
{
    return lsh.m_position != rhs.m_position;
}

}
