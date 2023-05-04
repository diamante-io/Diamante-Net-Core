#pragma once

// Copyright 2016 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "xdr/Diamnet-ledger-entries.h"
#include "xdr/Diamnet-transaction.h"
#include "xdr/Diamnet-types.h"

#include <map>
#include <set>
#include <stdint.h>
#include <vector>

namespace diamnet
{

class SignatureChecker
{
  public:
    explicit SignatureChecker(
        uint32_t protocolVersion, Hash const& contentsHash,
        xdr::xvector<DecoratedSignature, 20> const& signatures);

    bool checkSignature(AccountID const& accountID,
                        std::vector<Signer> const& signersV,
                        int32_t neededWeight);
    bool checkAllSignaturesUsed() const;

  private:
    uint32_t mProtocolVersion;
    Hash const& mContentsHash;
    xdr::xvector<DecoratedSignature, 20> const& mSignatures;

    std::vector<bool> mUsedSignatures;
};
};
