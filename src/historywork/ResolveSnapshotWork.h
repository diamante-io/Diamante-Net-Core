// Copyright 2015 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "work/Work.h"

namespace diamnet
{

struct StateSnapshot;

class ResolveSnapshotWork : public BasicWork
{
    std::shared_ptr<StateSnapshot> mSnapshot;
    std::unique_ptr<VirtualTimer> mTimer;
    asio::error_code mEc{asio::error_code()};

  public:
    ResolveSnapshotWork(Application& app,
                        std::shared_ptr<StateSnapshot> snapshot);
    ~ResolveSnapshotWork() = default;

  protected:
    State onRun() override;
    bool
    onAbort() override
    {
        return true;
    };
};
}
