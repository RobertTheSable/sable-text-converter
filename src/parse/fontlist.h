#ifndef FONTLIST_H
#define FONTLIST_H

#include "font/font.h"
#include <map>

namespace sable {

class FontList
{
private:
    typedef std::map<std::string, Font> Storage;
    Storage m_List;
public:
    struct iterator {
        iterator(Storage::const_iterator&& position);
        iterator &operator++();
        const Font* operator->() const;
        const Font& operator*() const;
        std::string getName() const;
        friend bool operator!=(const iterator& lsh, const iterator& rhs);
    private:
        Storage::const_iterator m_position;
    };
    FontList();
    void AddFont(const std::string& name, Font&& font);
    iterator begin() const;
    iterator end() const;
    const Font& operator[](const std::string& name) const;
    bool contains(const std::string& name) const;
    size_t size() const;
};
}


#endif // FONTLIST_H
