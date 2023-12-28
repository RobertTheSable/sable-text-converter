#include "file.h"
#include "exceptions.h"

namespace sable {
namespace files {

File::File(fs::path p): path{p}
{

}

std::ifstream File::open()
{
    return std::ifstream{path};
}

File::operator fs::path() const
{
    return path;
}

} // namespace files
} // namespace sable
