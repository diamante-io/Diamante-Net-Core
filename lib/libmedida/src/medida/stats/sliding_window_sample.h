// Copyright 2020 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#ifndef MEDIDA_SLIDING_WINDOW_SAMPLE_H_
#define MEDIDA_SLIDING_WINDOW_SAMPLE_H_

#include <cstdint>
#include <memory>

#include "medida/stats/sample.h"
#include "medida/types.h"

namespace medida
{
namespace stats
{

// Sliding window has both a size limit and time limit: samples are expired by
// time and/or size, with excess size-based expiries in a given time slice used
// to stochastically rate-limit further additions within that slice.

class SlidingWindowSample : public Sample
{
  public:
    SlidingWindowSample() = delete;
    SlidingWindowSample(std::size_t windowSize,
                        std::chrono::seconds windowTime);
    ~SlidingWindowSample();
    virtual void Seed(size_t);
    virtual void Clear();
    virtual std::uint64_t size() const;
    virtual void Update(std::int64_t value);
    virtual void Update(std::int64_t value, Clock::time_point timestamp);
    virtual Snapshot MakeSnapshot() const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace stats
} // namespace medida

#endif // MEDIDA_EXP_DECAY_SAMPLE_H_
