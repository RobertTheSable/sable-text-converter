#ifndef SABLE_MISSING_DATA_H
#define SABLE_MISSING_DATA_H

#include <stdexcept>
#include <string>

namespace sable {

struct MissingData : public std::runtime_error
{
    enum class Type {
        Table,
        File
    } type;
    std::string label;
    MissingData(Type type, const std::string label);
};

} // namespace sable

#endif // SABLE_MISSING_DATA_H
