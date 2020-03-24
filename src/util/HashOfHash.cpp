#include "HashOfHash.h"
#include "crypto/ByteSliceHasher.h"

namespace std
{

size_t
hash<HcNet::uint256>::operator()(HcNet::uint256 const& x) const noexcept
{
    size_t res =
        HcNet::shortHash::computeHash(HcNet::ByteSlice(x.data(), 8));

    return res;
}
}
