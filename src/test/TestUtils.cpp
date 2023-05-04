// Copyright 2016 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "TestUtils.h"
#include "overlay/test/LoopbackPeer.h"
#include "test/test.h"
#include "work/WorkScheduler.h"

namespace diamnet
{

namespace testutil
{

void
crankSome(VirtualClock& clock)
{
    auto start = clock.now();
    for (size_t i = 0;
         (i < 100 && clock.now() < (start + std::chrono::seconds(1)) &&
          clock.crank(false) > 0);
         ++i)
        ;
}

void
crankFor(VirtualClock& clock, VirtualClock::duration duration)
{
    auto start = clock.now();
    while (clock.now() < (start + duration) && clock.crank(false) > 0)
        ;
}

void
shutdownWorkScheduler(Application& app)
{
    if (app.getClock().getIOContext().stopped())
    {
        throw std::runtime_error("Work scheduler attempted to shutdown after "
                                 "VirtualClock io context stopped.");
    }
    app.getWorkScheduler().shutdown();
    while (app.getWorkScheduler().getState() != BasicWork::State::WORK_ABORTED)
    {
        app.getClock().crank();
    }
}

void
injectSendPeersAndReschedule(VirtualClock::time_point& end, VirtualClock& clock,
                             VirtualTimer& timer,
                             LoopbackPeerConnection& connection)
{
    connection.getInitiator()->sendGetPeers();
    if (clock.now() < end && connection.getInitiator()->isConnected())
    {
        timer.expires_from_now(std::chrono::milliseconds(10));
        timer.async_wait(
            [&]() {
                injectSendPeersAndReschedule(end, clock, timer, connection);
            },
            &VirtualTimer::onFailureNoop);
    }
}

BucketListDepthModifier::BucketListDepthModifier(uint32_t newDepth)
    : mPrevDepth(BucketList::kNumLevels)
{
    BucketList::kNumLevels = newDepth;
}

BucketListDepthModifier::~BucketListDepthModifier()
{
    BucketList::kNumLevels = mPrevDepth;
}
}

TestInvariantManager::TestInvariantManager(medida::MetricsRegistry& registry)
    : InvariantManagerImpl(registry)
{
}

void
TestInvariantManager::handleInvariantFailure(
    std::shared_ptr<Invariant> invariant, std::string const& message) const
{
    throw InvariantDoesNotHold{message};
}

TestApplication::TestApplication(VirtualClock& clock, Config const& cfg)
    : ApplicationImpl(clock, cfg)
{
}

std::unique_ptr<InvariantManager>
TestApplication::createInvariantManager()
{
    return std::make_unique<TestInvariantManager>(getMetrics());
}

time_t
getTestDate(int day, int month, int year)
{
    auto tm = getTestDateTime(day, month, year, 0, 0, 0);

    VirtualClock::system_time_point tp = VirtualClock::tmToSystemPoint(tm);
    time_t t = VirtualClock::to_time_t(tp);

    return t;
}

std::tm
getTestDateTime(int day, int month, int year, int hour, int minute, int second)
{
    std::tm tm = {0};
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_mday = day;
    tm.tm_mon = month - 1; // 0 based
    tm.tm_year = year - 1900;
    return tm;
}

VirtualClock::system_time_point
genesis(int minute, int second)
{
    return VirtualClock::tmToSystemPoint(
        getTestDateTime(1, 7, 2014, 0, minute, second));
}
}
