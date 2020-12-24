#pragma once
#include <xdr/DiamNet-types.h>

namespace std
{
template <> struct hash<DiamNet::uint256>
{
    size_t operator()(DiamNet::uint256 const& x) const noexcept;
};
}
