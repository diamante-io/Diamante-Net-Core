// Copyright 2020 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "medida/buckets.h"
#include "medida/timer.h"

#include <map>
#include <memory>

namespace medida
{
class Buckets::Impl
{
    std::map<double, std::shared_ptr<Timer>> mBuckets;
    const std::chrono::nanoseconds mDurationUnit;
    std::int64_t mDurationUnitNanos;

  public:
    Impl(std::set<double> const& bucketBoundaries,
         std::chrono::nanoseconds duration_unit,
         std::chrono::nanoseconds rate_unit)
        : mDurationUnit(duration_unit)
        , mDurationUnitNanos(duration_unit.count())
    {
        for (auto b: bucketBoundaries)
        {
            auto m = std::make_shared<Timer>(duration_unit, rate_unit);
            mBuckets.insert(std::make_pair(b, m));
        }
        mBuckets.insert(
            std::make_pair(std::numeric_limits<double>::max(),
                           std::make_shared<Timer>(duration_unit, rate_unit)));
    }

    std::map<double, std::shared_ptr<Timer>> const&
    getBuckets()
    {
        return mBuckets;
    }

    std::chrono::nanoseconds boundary_unit() const
    {
        return mDurationUnit;
    }

    void
    Update(std::chrono::nanoseconds value)
    {
        double v = double(value.count()) / mDurationUnitNanos;
        auto it = mBuckets.lower_bound(v);
        it->second->Update(value);
    }

    void Clear()
    {
        for (auto kv: mBuckets)
        {
            kv.second->Clear();
        }
    }
};

Buckets::Buckets(
    std::set<double> const& boundaries,
    std::chrono::nanoseconds duration_unit,
                 std::chrono::nanoseconds rate_unit)
    : impl_(new Buckets::Impl(boundaries, duration_unit, rate_unit))
{
}

Buckets::~Buckets()
{
}

void
Buckets::Process(MetricProcessor& processor)
{
    processor.Process(*this);
}

std::map<double, std::shared_ptr<Timer>> const&
Buckets::getBuckets()
{
    return impl_->getBuckets();
}

std::chrono::nanoseconds
Buckets::boundary_unit() const
{
    return impl_->boundary_unit();
}

void
Buckets::Update(std::chrono::nanoseconds value)
{
    impl_->Update(value);
}

void
Buckets::Clear()
{
    impl_->Clear();
}

}
