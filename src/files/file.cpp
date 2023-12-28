#include "file.h"
#include "exceptions.h"

namespace sable {
namespace files {

File::File(fs::path p): path{p}
{

}

std::ifstream File::open() const
{
    return std::ifstream{path};
}

void File::close(std::ifstream& in) const
{
    in.close();
}

File::operator fs::path() const
{
    return path;
}

} // namespace files
} // namespace sable
