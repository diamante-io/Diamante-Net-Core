#pragma once

// Copyright 2018 DiamNet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "crypto/ByteSliceHasher.h"
#include "xdr/DiamNet-ledger.h"
#include <functional>

// implements a default hasher for "LedgerKey"
namespace std
{
template <> class hash<DiamNet::LedgerKey>
{
  public:
    size_t
    operator()(DiamNet::LedgerKey const& lk) const
    {
        size_t res;
        switch (lk.type())
        {
        case DiamNet::ACCOUNT:
            res = DiamNet::shortHash::computeHash(
                DiamNet::ByteSlice(lk.account().accountID.ed25519().data(), 8));
            break;
        case DiamNet::TRUSTLINE:
        {
            auto& tl = lk.trustLine();
            res = DiamNet::shortHash::computeHash(
                DiamNet::ByteSlice(tl.accountID.ed25519().data(), 8));
            switch (lk.trustLine().asset.type())
            {
            case DiamNet::ASSET_TYPE_NATIVE:
                break;
            case DiamNet::ASSET_TYPE_CREDIT_ALPHANUM4:
            {
                auto& tl4 = tl.asset.alphaNum4();
                res ^= DiamNet::shortHash::computeHash(
                    DiamNet::ByteSlice(tl4.issuer.ed25519().data(), 8));
                res ^= tl4.assetCode[0];
                break;
            }
            case DiamNet::ASSET_TYPE_CREDIT_ALPHANUM12:
            {
                auto& tl12 = tl.asset.alphaNum12();
                res ^= DiamNet::shortHash::computeHash(
                    DiamNet::ByteSlice(tl12.issuer.ed25519().data(), 8));
                res ^= tl12.assetCode[0];
                break;
            }
            default:
                abort();
            }
            break;
        }
        case DiamNet::DATA:
            res = DiamNet::shortHash::computeHash(
                DiamNet::ByteSlice(lk.data().accountID.ed25519().data(), 8));
            res ^= DiamNet::shortHash::computeHash(DiamNet::ByteSlice(
                lk.data().dataName.data(), lk.data().dataName.size()));
            break;
        case DiamNet::OFFER:
            res = DiamNet::shortHash::computeHash(DiamNet::ByteSlice(
                &lk.offer().offerID, sizeof(lk.offer().offerID)));
            break;
        default:
            abort();
        }
        return res;
    }
};
}
