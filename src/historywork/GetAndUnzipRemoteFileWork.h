// Copyright 2015 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "history/FileTransferInfo.h"
#include "work/Work.h"
#include <medida/meter.h>
#include <medida/metrics_registry.h>

namespace diamnet
{

class HistoryArchive;

class GetAndUnzipRemoteFileWork : public Work
{
    std::shared_ptr<BasicWork> mGetRemoteFileWork;
    std::shared_ptr<BasicWork> mGunzipFileWork;

    FileTransferInfo mFt;
    std::shared_ptr<HistoryArchive> mArchive;

    medida::Meter& mDownloadStart;
    medida::Meter& mDownloadSuccess;
    medida::Meter& mDownloadFailure;

    bool validateFile();

  public:
    // Passing `nullptr` for the archive argument will cause the work to
    // select a new readable history archive at random each time it runs /
    // retries.
    GetAndUnzipRemoteFileWork(
        Application& app, FileTransferInfo ft,
        std::shared_ptr<HistoryArchive> archive = nullptr);
    ~GetAndUnzipRemoteFileWork() = default;
    std::string getStatus() const override;

  protected:
    void doReset() override;
    void onFailureRaise() override;
    void onSuccess() override;
    State doWork() override;
};
}
