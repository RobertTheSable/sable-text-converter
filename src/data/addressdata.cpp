#include "address.h"

#include <sstream>
#include <iomanip>

namespace sable {

AddressNode::operator std::string() const
{
    std::ostringstream ss;
    ss << (isTable ? "Table " : "Address ") << (label + ": ")  << std::hex << address;
    return ss.str();
}

}
