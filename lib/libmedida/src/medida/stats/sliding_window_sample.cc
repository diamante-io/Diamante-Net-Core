// Copyright 2020 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "medida/stats/sliding_window_sample.h"

#include <cassert>
#include <chrono>
#include <deque>
#include <mutex>
#include <random>

#include "medida/stats/snapshot.h"

namespace medida
{
namespace stats
{

class SlidingWindowSample::Impl
{
  public:
    Impl(std::size_t windowSize, std::chrono::seconds windowTime);
    ~Impl();
    void Clear();
    void Seed(size_t seed);
    std::uint64_t size();
    void Update(std::int64_t value);
    void Update(std::int64_t value, Clock::time_point timestamp);
    Snapshot MakeSnapshot();

  private:
    std::mutex mutex_;
    const std::size_t windowSize_;
    const std::chrono::seconds windowTime_;
    const std::chrono::microseconds timeSlice_;
    std::uint32_t samplesInCurrentSlice_;
    std::default_random_engine rng_;
    std::uniform_int_distribution<std::uint32_t> dist_;
    std::deque<std::pair<double, Clock::time_point>> values_;
};

SlidingWindowSample::SlidingWindowSample(std::size_t windowSize,
                                         std::chrono::seconds windowTime)
    : impl_{new SlidingWindowSample::Impl{windowSize, windowTime}}
{
}

SlidingWindowSample::~SlidingWindowSample()
{
}

void
SlidingWindowSample::Clear()
{
    impl_->Clear();
}

void
SlidingWindowSample::Seed(size_t seed)
{
    impl_->Seed(seed);
}

std::uint64_t
SlidingWindowSample::size() const
{
    return impl_->size();
}

void
SlidingWindowSample::Update(std::int64_t value)
{
    impl_->Update(value);
}

void
SlidingWindowSample::Update(std::int64_t value, Clock::time_point timestamp)
{
    impl_->Update(value, timestamp);
}

Snapshot
SlidingWindowSample::MakeSnapshot() const
{
    return impl_->MakeSnapshot();
}

// === Implementation ===

SlidingWindowSample::Impl::Impl(std::size_t windowSize,
                                std::chrono::seconds windowTime)
    : windowSize_(windowSize)
    , windowTime_(windowTime)
    , timeSlice_(
          std::chrono::duration_cast<std::chrono::microseconds>(windowTime) /
          windowSize)
    , samplesInCurrentSlice_(0)
    , rng_(std::random_device()())
    , dist_(0, std::numeric_limits<std::uint32_t>::max())
{
    Clear();
}

SlidingWindowSample::Impl::~Impl()
{
}

void
SlidingWindowSample::Impl::Seed(size_t seed)
{
    std::lock_guard<std::mutex> lock{mutex_};
    rng_.seed(static_cast<std::uint32_t>(seed));
}

void
SlidingWindowSample::Impl::Clear()
{
    std::lock_guard<std::mutex> lock{mutex_};
    values_.clear();
}

std::uint64_t
SlidingWindowSample::Impl::size()
{
    std::lock_guard<std::mutex> lock{mutex_};
    return values_.size();
}

void
SlidingWindowSample::Impl::Update(std::int64_t value)
{
    Update(value, Clock::now());
}

void
SlidingWindowSample::Impl::Update(std::int64_t value,
                                  Clock::time_point timestamp)
{
    std::lock_guard<std::mutex> lock{mutex_};

    if (!values_.empty())
    {
        // If we're in a new timeslice, reset count
        if (timestamp > values_.back().second + timeSlice_)
        {
            samplesInCurrentSlice_ = 0;
        }

        // If there's old data, trim it.
        Clock::time_point expiryTime = timestamp - windowTime_;
        while (!values_.empty() && values_.front().second < expiryTime)
        {
            values_.pop_front();
        }
    }

    // When you add samples to the sliding window _slowly_ nothing goes wrong;
    // when you add them too _quickly_ there's the possibility of losing rare
    // events because they're overwritten before they get observed.
    //
    // To compensate for this, we divide the total fixed-duration time window by
    // the fixed number of samples we want to retain, resulting in fixed-size
    // _timeslices_. And then within each timeslice we arrange to keep a random
    // representative of the samples that arrive during that slice.

    // Check if we've already inserted an item for the same timeSlice.
    if (!values_.empty() && timestamp <= values_.back().second + timeSlice_)
    {
        // Here we're trying to cheaply (i.e. using only integer ops) calculate
        // a condition that results in each of N samples being chosen with
        // probability 1/N. Since we don't know N in advance, only as time
        // proceeds, we achieve the goal by tracking the count K of samples
        // we've received in a timeslice and repeatedly _replacing_ the Kth
        // sample with probability 1/K.
        //
        // Proof that this behaviour is correct is by induction:
        //
        //  - When K = 1 obviously it is correct: 1/1 = 1/K = 1/N
        //
        //  - Now for case K, let J = K-1, and assume for N = J that the
        //    replacement behaviour is correct, meaning that every sample so far
        //    has an equal 1/J chance of being the current surviving
        //    candidate. Now we replace that candidate with probability 1/K. The
        //    new sample obviously has odds of being the new survivor with odds
        //    1/K, and the previous survivor has odds 1/J * (1 - 1/K)
        //
        //      = 1/J - 1/JK         -- distributing
        //      = K/JK - 1/JK        -- taking common denominator
        //      = (K-1)/JK           -- applying -
        //      = (K-1)/((K-1)K)     -- expanding definition of J
        //      = 1/K                -- reducing fraction
        //
        //    Since the previous survivor is _any_ of the previous J samples,
        //    they all now have the same 1/K chance of surviving this current
        //    replacement.
        //
        // Next: to _accomplish_ replacement with probability 1/K, given K as
        // the count of events so far (samplesInCurrentSlice_), we take M as the
        // maximum uint32_t and R a random uint32_t, and check R * K <= M (with
        // all values promoted to 64-bit so they won't overflow).
        //
        // Algebraically (if we were using real numbers) this is the same as
        // checking RK / M <= 1 or R/M <= 1/K, where the R/M term varies between
        // 0 and 1. When R is uniformly random, this condition is true exactly
        // 1/K of the time, which is what we're after; and it can be calculated
        // faithfully using integers when we write it as R*K <= M.

        samplesInCurrentSlice_++;
        uint32_t r = dist_(rng_);
        uint64_t rk = uint64_t(r) * uint64_t(samplesInCurrentSlice_);
        uint64_t m = uint64_t(std::numeric_limits<std::uint32_t>::max());
        if (rk <= m)
        {
            // Keep old timestamp to anchor timeSlice; but replace value.
            values_.back().first = value;
        }
    }
    else
    {
        values_.emplace_back(value, timestamp);
        samplesInCurrentSlice_ = 1;
        if (values_.size() > windowSize_)
        {
            values_.pop_front();
        }
    }
}

Snapshot
SlidingWindowSample::Impl::MakeSnapshot()
{
    std::lock_guard<std::mutex> lock{mutex_};
    std::vector<double> vals;
    vals.reserve(values_.size());
    for (auto v : values_)
    {
        vals.emplace_back(v.first);
    }
    return {vals};
}

} // namespace stats
} // namespace medida
