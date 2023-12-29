#include "textblockrange.h"

#include <tuple>

namespace sable {

bool operator==(const TextBlockRange& lhs, const TextBlockRange& rhs)
{
    return (lhs.start == rhs.start) && (lhs.end == rhs.end);
}

bool operator<(const TextBlockRange& lhs, const TextBlockRange& rhs)
{
    return (lhs.start < rhs.start) || (lhs.end < rhs.end);
}

bool operator==(const Blocks::Collision& lhs, const Blocks::CollidesWith& rhs)
{
    return lhs == rhs.type;
}

bool operator==(const Blocks::CollidesWith& lhs, const Blocks::Collision& rhs)
{
    return lhs.type == rhs;
}

Blocks::CollidesWith Blocks::addBlock(TextBlockRange&& newRange)
{
    using BlockIterator = decltype(blocks)::iterator;
    bool inserted = false;
    BlockIterator position = blocks.end();
    std::tie(position, inserted) = blocks.insert(newRange);
    if (!inserted) {
        return {Collision::Same, *position};
    }
    if (position != blocks.begin()) {
        auto prior = position;
        --prior;
        if (prior->end > position->start) {
            return {Collision::Prior, *prior};
        }
    }
    auto next = position;
    ++next;
    if (next != blocks.end()) {
        if (position->end > next->start) {
            return {Collision::Next, *next};
        }
    }

    return {Collision::None, *position};
}

TextBlockRange* Blocks::CollidesWith::operator->()
{
    return &data;
}

}
