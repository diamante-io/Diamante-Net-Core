#pragma once

// Copyright 2017 DiamNet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "xdr/DiamNet-types.h"
#include <vector>

namespace DiamNet
{

struct SCPEnvelope;
struct SCPStatement;
struct DiamNetValue;

std::vector<Hash> getTxSetHashes(SCPEnvelope const& envelope);
std::vector<DiamNetValue> getDiamNetValues(SCPStatement const& envelope);
}
