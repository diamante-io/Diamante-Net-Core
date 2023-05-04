// Copyright 2020 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "util/XDRCereal.h"

void
cereal_override(cereal::JSONOutputArchive& ar, const diamnet::PublicKey& s,
                const char* field)
{
    xdr::archive(ar, diamnet::KeyUtils::toStrKey<diamnet::PublicKey>(s), field);
}

void
cereal_override(cereal::JSONOutputArchive& ar,
                const diamnet::MuxedAccount& muxedAccount, const char* field)
{
    switch (muxedAccount.type())
    {
    case diamnet::KEY_TYPE_ED25519:
        xdr::archive(ar, diamnet::KeyUtils::toStrKey(toAccountID(muxedAccount)),
                     field);
        return;
    case diamnet::KEY_TYPE_MUXED_ED25519:
        xdr::archive(
            ar,
            std::make_tuple(
                cereal::make_nvp("id", muxedAccount.med25519().id),
                cereal::make_nvp("accountID", diamnet::KeyUtils::toStrKey(
                                                  toAccountID(muxedAccount)))),
            field);
        return;
    default:
        // this would be a bug
        abort();
    }
}

void
cereal_override(cereal::JSONOutputArchive& ar, const diamnet::Asset& s,
                const char* field)
{
    xdr::archive(ar, diamnet::assetToString(s), field);
}
