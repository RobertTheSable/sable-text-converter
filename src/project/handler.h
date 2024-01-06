#ifndef HANDLER_H
#define HANDLER_H

#include "parse/parse.h"
#include "data/addresslist.h"
#include "data/options.h"

#include "exceptions.h"

#include "errorhandling.h"
#include "wrapper/filesystem.h"

namespace sable {

struct Handler: sable::Parser<Handler>
{
    AddressList addresses;
    fs::path baseDir;
    std::ostream& output;

    template<typename ...Args>
    Handler(fs::path dir_, std::ostream& out, Args&& ...args): baseDir{dir_}, output{out}, sable::Parser<Handler>(std::forward<Args>(args)...) {

    }
    void report(
        std::string file,
        error::Levels l,
        std::string msg,
        int line
    );

    void write (
        std::string fileName,
        std::string label,
        const std::vector<unsigned char>& data,
        int address,
        size_t start,
        size_t length,
        bool printpc,
        options::ExportWidth exportWidth,
        options::ExportAddress exportAddress
    );


    AddressList done();
    int getNextAddress(const std::string & dir) const;
    void setNextAddress(int nextAddress);
};

}

#endif // HANDLER_H
