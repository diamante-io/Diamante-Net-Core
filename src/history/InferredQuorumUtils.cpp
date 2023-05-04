// Copyright 2018 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "herder/Herder.h"
#include "herder/QuorumIntersectionChecker.h"
#include "history/HistoryManager.h"
#include "main/Application.h"
#include "main/Config.h"
#include "util/Logging.h"
#include "util/XDROperators.h"
#include "xdr/Diamnet-SCP.h"
#include "xdr/Diamnet-types.h"

#include <algorithm>
#include <set>

namespace diamnet
{

static diamnet::QuorumTracker::QuorumMap
getQuorumMapForLedger(Application::pointer app, uint32_t ledgerNum)
{
    if (ledgerNum == 0)
    {
        return app->getHerder().getCurrentlyTrackedQuorum();
    }
    else
    {
        return app->getHistoryManager().inferQuorum(ledgerNum).getQuorumMap();
    }
}

void
checkQuorumIntersection(Config const& cfg, uint32_t ledgerNum)
{
    VirtualClock clock;
    Config cfg2(cfg);
    cfg2.setNoListen();
    Application::pointer app = Application::create(clock, cfg2, false);
    LOG(INFO) << "Checking last-heard quorum from herder";
    app->start();
    auto qmap = getQuorumMapForLedger(app, ledgerNum);
    std::atomic<bool> interruptFlag{false};
    auto qic = QuorumIntersectionChecker::create(qmap, cfg, interruptFlag);
    qic->networkEnjoysQuorumIntersection();
}

void
inferQuorumAndWrite(Config const& cfg, uint32_t ledgerNum)
{
    Config cfg2(cfg);
    InferredQuorum iq;
    {
        VirtualClock clock;
        cfg2.setNoListen();
        Application::pointer app = Application::create(clock, cfg2, false);
        auto qmap = getQuorumMapForLedger(app, ledgerNum);
        iq = InferredQuorum(qmap);
    }
    LOG(INFO) << "Inferred quorum";
    std::cout << iq.toString(cfg2, true) << std::endl;
}

void
writeQuorumGraph(Config const& cfg, std::string const& outputFile,
                 uint32_t ledgerNum)
{
    Config cfg2(cfg);
    InferredQuorum iq;
    {
        VirtualClock clock;
        cfg2.setNoListen();
        Application::pointer app = Application::create(clock, cfg2, false);
        auto qmap = getQuorumMapForLedger(app, ledgerNum);
        iq = InferredQuorum(qmap);
    }
    std::string filename = outputFile.empty() ? "-" : outputFile;
    if (filename == "-")
    {
        std::stringstream out;
        iq.writeQuorumGraph(cfg2, out);
        LOG(INFO) << "*";
        LOG(INFO) << "* Quorum graph: " << out.str();
        LOG(INFO) << "*";
    }
    else
    {
        std::ofstream out;
        out.exceptions(std::ios::failbit | std::ios::badbit);
        out.open(filename);
        iq.writeQuorumGraph(cfg2, out);
        LOG(INFO) << "*";
        LOG(INFO) << "* Wrote quorum graph to " << filename;
        LOG(INFO) << "*";
    }
}
}
