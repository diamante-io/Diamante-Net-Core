#pragma once

// Copyright 2018 HcNet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "crypto/ByteSliceHasher.h"
#include "xdr/HcNet-ledger.h"
#include <functional>

// implements a default hasher for "LedgerKey"
namespace std
{
template <> class hash<HcNet::LedgerKey>
{
  public:
    size_t
    operator()(HcNet::LedgerKey const& lk) const
    {
        size_t res;
        switch (lk.type())
        {
        case HcNet::ACCOUNT:
            res = HcNet::shortHash::computeHash(
                HcNet::ByteSlice(lk.account().accountID.ed25519().data(), 8));
            break;
        case HcNet::TRUSTLINE:
        {
            auto& tl = lk.trustLine();
            res = HcNet::shortHash::computeHash(
                HcNet::ByteSlice(tl.accountID.ed25519().data(), 8));
            switch (lk.trustLine().asset.type())
            {
            case HcNet::ASSET_TYPE_NATIVE:
                break;
            case HcNet::ASSET_TYPE_CREDIT_ALPHANUM4:
            {
                auto& tl4 = tl.asset.alphaNum4();
                res ^= HcNet::shortHash::computeHash(
                    HcNet::ByteSlice(tl4.issuer.ed25519().data(), 8));
                res ^= tl4.assetCode[0];
                break;
            }
            case HcNet::ASSET_TYPE_CREDIT_ALPHANUM12:
            {
                auto& tl12 = tl.asset.alphaNum12();
                res ^= HcNet::shortHash::computeHash(
                    HcNet::ByteSlice(tl12.issuer.ed25519().data(), 8));
                res ^= tl12.assetCode[0];
                break;
            }
            default:
                abort();
            }
            break;
        }
        case HcNet::DATA:
            res = HcNet::shortHash::computeHash(
                HcNet::ByteSlice(lk.data().accountID.ed25519().data(), 8));
            res ^= HcNet::shortHash::computeHash(HcNet::ByteSlice(
                lk.data().dataName.data(), lk.data().dataName.size()));
            break;
        case HcNet::OFFER:
            res = HcNet::shortHash::computeHash(HcNet::ByteSlice(
                &lk.offer().offerID, sizeof(lk.offer().offerID)));
            break;
        default:
            abort();
        }
        return res;
    }
};
}
