#pragma once

// Copyright 2018 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "crypto/ShortHash.h"
#include "ledger/GeneralizedLedgerEntry.h"
#include "xdr/Diamnet-ledger.h"
#include <functional>

// implements a default hasher for "LedgerKey"
namespace std
{
template <> class hash<diamnet::Asset>
{
  public:
    size_t
    operator()(diamnet::Asset const& asset) const
    {
        size_t res = asset.type();
        switch (asset.type())
        {
        case diamnet::ASSET_TYPE_NATIVE:
            break;
        case diamnet::ASSET_TYPE_CREDIT_ALPHANUM4:
        {
            auto& a4 = asset.alphaNum4();
            res ^= diamnet::shortHash::computeHash(
                diamnet::ByteSlice(a4.issuer.ed25519().data(), 8));
            res ^= a4.assetCode[0];
            break;
        }
        case diamnet::ASSET_TYPE_CREDIT_ALPHANUM12:
        {
            auto& a12 = asset.alphaNum12();
            res ^= diamnet::shortHash::computeHash(
                diamnet::ByteSlice(a12.issuer.ed25519().data(), 8));
            res ^= a12.assetCode[0];
            break;
        }
        }
        return res;
    }
};

template <> class hash<diamnet::LedgerKey>
{
  public:
    size_t
    operator()(diamnet::LedgerKey const& lk) const
    {
        size_t res;
        switch (lk.type())
        {
        case diamnet::ACCOUNT:
            res = diamnet::shortHash::computeHash(
                diamnet::ByteSlice(lk.account().accountID.ed25519().data(), 8));
            break;
        case diamnet::TRUSTLINE:
        {
            auto& tl = lk.trustLine();
            res = diamnet::shortHash::computeHash(
                diamnet::ByteSlice(tl.accountID.ed25519().data(), 8));
            res ^= hash<diamnet::Asset>()(tl.asset);
            break;
        }
        case diamnet::DATA:
            res = diamnet::shortHash::computeHash(
                diamnet::ByteSlice(lk.data().accountID.ed25519().data(), 8));
            res ^= diamnet::shortHash::computeHash(diamnet::ByteSlice(
                lk.data().dataName.data(), lk.data().dataName.size()));
            break;
        case diamnet::OFFER:
            res = diamnet::shortHash::computeHash(diamnet::ByteSlice(
                &lk.offer().offerID, sizeof(lk.offer().offerID)));
            break;
        case diamnet::CLAIMABLE_BALANCE:
            res = diamnet::shortHash::computeHash(diamnet::ByteSlice(
                lk.claimableBalance().balanceID.v0().data(), 8));
            break;
        default:
            abort();
        }
        return res;
    }
};

template <> class hash<diamnet::GeneralizedLedgerKey>
{
  public:
    size_t
    operator()(diamnet::GeneralizedLedgerKey const& glk) const
    {
        switch (glk.type())
        {
        case diamnet::GeneralizedLedgerEntryType::LEDGER_ENTRY:
            return hash<diamnet::LedgerKey>()(glk.ledgerKey());
        case diamnet::GeneralizedLedgerEntryType::SPONSORSHIP:
            return diamnet::shortHash::computeHash(diamnet::ByteSlice(
                glk.sponsorshipKey().sponsoredID.ed25519().data(), 8));
        case diamnet::GeneralizedLedgerEntryType::SPONSORSHIP_COUNTER:
            return diamnet::shortHash::computeHash(diamnet::ByteSlice(
                glk.sponsorshipCounterKey().sponsoringID.ed25519().data(), 8));
        default:
            abort();
        }
    }
};
}
