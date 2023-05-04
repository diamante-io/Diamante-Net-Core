#include "HashOfHash.h"
#include "crypto/ShortHash.h"

namespace std
{

size_t
hash<diamnet::uint256>::operator()(diamnet::uint256 const& x) const noexcept
{
    size_t res =
        diamnet::shortHash::computeHash(diamnet::ByteSlice(x.data(), 8));

    return res;
}
}
