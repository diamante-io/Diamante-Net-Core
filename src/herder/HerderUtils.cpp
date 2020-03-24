// Copyright 2014 HcNet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "herder/HerderUtils.h"
#include "scp/Slot.h"
#include "xdr/HcNet-ledger.h"
#include <algorithm>
#include <xdrpp/marshal.h>

namespace HcNet
{

std::vector<Hash>
getTxSetHashes(SCPEnvelope const& envelope)
{
    auto values = getHcNetValues(envelope.statement);
    auto result = std::vector<Hash>{};
    result.resize(values.size());

    std::transform(std::begin(values), std::end(values), std::begin(result),
                   [](HcNetValue const& sv) { return sv.txSetHash; });

    return result;
}

std::vector<HcNetValue>
getHcNetValues(SCPStatement const& statement)
{
    auto values = Slot::getStatementValues(statement);
    auto result = std::vector<HcNetValue>{};
    result.resize(values.size());

    std::transform(std::begin(values), std::end(values), std::begin(result),
                   [](Value const& v) {
                       auto wb = HcNetValue{};
                       xdr::xdr_from_opaque(v, wb);
                       return wb;
                   });

    return result;
}
}
