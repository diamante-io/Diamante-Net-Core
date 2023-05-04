// Copyright 2015 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "historywork/GetHistoryArchiveStateWork.h"
#include "history/HistoryArchive.h"
#include "historywork/GetRemoteFileWork.h"
#include "ledger/LedgerManager.h"
#include "main/Application.h"
#include "main/ErrorMessages.h"
#include "util/Logging.h"
#include <Tracy.hpp>
#include <fmt/format.h>
#include <medida/meter.h>
#include <medida/metrics_registry.h>

namespace diamnet
{
GetHistoryArchiveStateWork::GetHistoryArchiveStateWork(
    Application& app, uint32_t seq, std::shared_ptr<HistoryArchive> archive,
    std::string mode, size_t maxRetries)
    : Work(app, "get-archive-state", maxRetries)
    , mSeq(seq)
    , mArchive(archive)
    , mRetries(maxRetries)
    , mLocalFilename(
          archive ? HistoryArchiveState::localName(app, archive->getName())
                  : app.getHistoryManager().localFilename(
                        HistoryArchiveState::baseName()))
    , mGetHistoryArchiveStateSuccess(app.getMetrics().NewMeter(
          {"history", "download-history-archive-state" + std::move(mode),
           "success"},
          "event"))
{
}

BasicWork::State
GetHistoryArchiveStateWork::doWork()
{
    ZoneScoped;
    if (mGetRemoteFile)
    {
        auto state = mGetRemoteFile->getState();
        if (state == State::WORK_SUCCESS)
        {
            try
            {
                mState.load(mLocalFilename);
            }
            catch (std::runtime_error& e)
            {
                CLOG(ERROR, "History")
                    << "Error loading history state: " << e.what();
                CLOG(ERROR, "History") << POSSIBLY_CORRUPTED_LOCAL_FS;
                CLOG(ERROR, "History") << "OR";
                CLOG(ERROR, "History") << POSSIBLY_CORRUPTED_HISTORY;
                CLOG(ERROR, "History") << "OR";
                CLOG(ERROR, "History") << UPGRADE_DIAMNET_CORE;
                return State::WORK_FAILURE;
            }
        }
        return state;
    }

    else
    {
        auto name = getRemoteName();
        CLOG(INFO, "History") << "Downloading history archive state: " << name;
        mGetRemoteFile = addWork<GetRemoteFileWork>(name, mLocalFilename,
                                                    mArchive, mRetries);
        return State::WORK_RUNNING;
    }
}

void
GetHistoryArchiveStateWork::doReset()
{
    mGetRemoteFile.reset();
    std::remove(mLocalFilename.c_str());
    mState = {};
}

void
GetHistoryArchiveStateWork::onSuccess()
{
    mGetHistoryArchiveStateSuccess.Mark();
    Work::onSuccess();
}

std::string
GetHistoryArchiveStateWork::getRemoteName() const
{
    return mSeq == 0 ? HistoryArchiveState::wellKnownRemoteName()
                     : HistoryArchiveState::remoteName(mSeq);
}

std::string
GetHistoryArchiveStateWork::getStatus() const
{
    std::string ledgerString = mSeq == 0 ? "current" : std::to_string(mSeq);
    return fmt::format("Downloading state file {} for ledger {}",
                       getRemoteName(), ledgerString);
}
}
