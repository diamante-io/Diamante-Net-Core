#pragma once
#include <xdr/Diamnet-types.h>

namespace std
{
template <> struct hash<diamnet::uint256>
{
    size_t operator()(diamnet::uint256 const& x) const noexcept;
};
}
