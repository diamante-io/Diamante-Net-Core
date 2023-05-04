#pragma once

// Copyright 2018 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include <string>

namespace diamnet
{

class Config;

void checkQuorumIntersection(Config const& cfg, uint32_t ledgerNum);
void inferQuorumAndWrite(Config const& cfg, uint32_t ledgerNum);
void writeQuorumGraph(Config const& cfg, std::string const& outputFile,
                      uint32_t ledgerNum);
}
