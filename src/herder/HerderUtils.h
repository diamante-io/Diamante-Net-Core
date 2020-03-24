#pragma once

// Copyright 2017 HcNet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "xdr/HcNet-types.h"
#include <vector>

namespace HcNet
{

struct SCPEnvelope;
struct SCPStatement;
struct HcNetValue;

std::vector<Hash> getTxSetHashes(SCPEnvelope const& envelope);
std::vector<HcNetValue> getHcNetValues(SCPStatement const& envelope);
}
