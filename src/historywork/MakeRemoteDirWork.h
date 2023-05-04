// Copyright 2015 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "historywork/RunCommandWork.h"

namespace diamnet
{

class HistoryArchive;

class MakeRemoteDirWork : public RunCommandWork
{
    std::string const mDir;
    std::shared_ptr<HistoryArchive> mArchive;
    CommandInfo getCommand() override;

  public:
    MakeRemoteDirWork(Application& app, std::string const& dir,
                      std::shared_ptr<HistoryArchive> archive);
    ~MakeRemoteDirWork() = default;

    void onSuccess() override;
    void onFailureRaise() override;
};
}
