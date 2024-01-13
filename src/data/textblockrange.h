#ifndef TEXTBLOCKRANGE_H
#define TEXTBLOCKRANGE_H

#include <cstdint>
#include <set>
#include <string>

namespace sable {

struct TextBlockRange
{
    int start;
    std::size_t end;
    std::string label, file;
    friend bool operator==(const TextBlockRange& lhs, const TextBlockRange& rhs);
    friend bool operator<(const TextBlockRange& lhs, const TextBlockRange& rhs);
};

class Blocks {
    std::set<TextBlockRange> blocks;
public:
    enum class Collision {None, Same, Prior, Next};
    struct CollidesWith {
        Collision type;
        TextBlockRange data;
        friend bool operator==(const Collision& lhs, const CollidesWith& rhs);
        friend bool operator==(const CollidesWith& lhs, const Collision& rhs);
        template<typename Arg1, typename Arg2>
        friend bool operator!=(Arg1&& lhs, Arg2&& rhs) {
            return ! (lhs == rhs);
        }
        TextBlockRange* operator->();
    };

    CollidesWith addBlock(TextBlockRange&& newRange);
    template<typename ...Args>
    CollidesWith addBlock(Args&& ...args) {
        return addBlock(TextBlockRange{args...});
    }
};

}

#endif // TEXTBLOCKRANGE_H
