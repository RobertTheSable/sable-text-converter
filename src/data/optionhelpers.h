#ifndef OPTIONHELPERS_H
#define OPTIONHELPERS_H

#include <string>
#include <stdexcept>

namespace sable {

namespace options {

template<class En>
En parseBool(std::string name, std::string option)
{
    if (option == "on") {
        return En::On;
    } else if (option == "off") {
        return En::Off;
    } else {
        throw std::runtime_error(option.insert(0, "Invalid option \"") + "\" for " + name + ": must be on or off.");
    }
}

template<class En>
bool isEnabled(En option) {
    return option == En::On;
}

}
}

#endif // OPTIONHELPERS_H
