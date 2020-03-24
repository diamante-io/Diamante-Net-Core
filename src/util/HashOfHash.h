#pragma once
#include <xdr/HcNet-types.h>

namespace std
{
template <> struct hash<HcNet::uint256>
{
    size_t operator()(HcNet::uint256 const& x) const noexcept;
};
}
