#pragma once

// Copyright 2017 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "xdr/Diamnet-types.h"
#include <vector>

namespace diamnet
{

struct SCPEnvelope;
struct SCPStatement;
struct DiamnetValue;

std::vector<Hash> getTxSetHashes(SCPEnvelope const& envelope);
std::vector<DiamnetValue> getDiamnetValues(SCPStatement const& envelope);
}
