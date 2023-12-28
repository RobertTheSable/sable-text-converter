#ifndef RESULT_H
#define RESULT_H

namespace sable {
namespace parse {

struct Result {
    enum class Status {Done, NotDone, Error};
    bool done;
    int length;
    int line;
};

struct FileResult {
    int dirIndex, address;
};

}

}

#endif // RESULT_H
