#ifndef CODENOTFOUND_H
#define CODENOTFOUND_H

#include <stdexcept>

namespace sable {

class CodeNotFound : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}

#endif // CODENOTFOUND_H
