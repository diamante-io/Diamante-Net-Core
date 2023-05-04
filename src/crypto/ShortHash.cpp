// Copyright 2018 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "ShortHash.h"
#include <sodium.h>

namespace diamnet
{
namespace shortHash
{
static unsigned char sKey[crypto_shorthash_KEYBYTES];
void
initialize()
{
    crypto_shorthash_keygen(sKey);
}
uint64_t
computeHash(diamnet::ByteSlice const& b)
{
    uint64_t res;
    static_assert(sizeof(res) == crypto_shorthash_BYTES, "unexpected size");
    crypto_shorthash(reinterpret_cast<unsigned char*>(&res),
                     reinterpret_cast<const unsigned char*>(b.data()), b.size(),
                     sKey);
    return res;
}

XDRShortHasher::XDRShortHasher() : state(sKey)
{
}

void
XDRShortHasher::hashBytes(unsigned char const* bytes, size_t len)
{
    state.update(bytes, len);
}
}
}
