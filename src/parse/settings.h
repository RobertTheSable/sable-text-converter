#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include "data/options.h"

namespace sable {

struct ParseSettings {
    enum class Autoend {
        On, Off
    } autoend;
    bool printpc;
    std::string mode, label;
    int maxWidth;
    int currentAddress;
    int page;
    enum class EndOnLabel {
        On, Off
    } endOnLabel;
    sable::options::ExportAddress exportAddress;
    sable::options::ExportWidth exportWidth;
};

}
#endif // SETTINGS_H
