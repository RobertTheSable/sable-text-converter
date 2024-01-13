#include "missing_data.h"

namespace sable {

auto buildWhat(MissingData::Type type, std::string label)
{
    return [&type] () -> std::string {
        if (type == MissingData::Type::File) {
            return "Label";
        } else {
            return "Table";
        }
    }() +" \"" + label + "\" not found.";
}

MissingData::MissingData(Type type_, const std::string label_)
    : type{type_}, label{label_}, std::runtime_error(buildWhat(type_, label_))
{

}

} // namespace sable
