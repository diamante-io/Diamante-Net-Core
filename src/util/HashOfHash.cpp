#include "HashOfHash.h"
#include "crypto/ByteSliceHasher.h"

namespace std
{

size_t
hash<DiamNet::uint256>::operator()(DiamNet::uint256 const& x) const noexcept
{
    size_t res =
        DiamNet::shortHash::computeHash(DiamNet::ByteSlice(x.data(), 8));

    return res;
}
}
