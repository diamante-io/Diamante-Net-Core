//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/stats/exp_decay_sample.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <map>
#include <mutex>
#include <random>

#include "medida/stats/snapshot.h"

namespace medida {
namespace stats {

static const Clock::duration kRESCALE_THRESHOLD = std::chrono::hours{1};

class ExpDecaySample::Impl {
 public:
  Impl(std::uint32_t reservoirSize, double alpha);
  ~Impl();
  void Clear();
  std::uint64_t size() const;
  void Update(std::int64_t value);
  void Update(std::int64_t value, Clock::time_point timestamp);
  Snapshot MakeSnapshot() const;
 private:
  const double alpha_;
  const std::uint64_t reservoirSize_;
  Clock::time_point startTime_;
  Clock::time_point nextScaleTime_;

  std::atomic<std::uint64_t> count_;
  std::map<double, std::int64_t> values_;
  std::mutex mutex_;
  mutable std::mt19937 rng_;
  std::uniform_real_distribution<> dist_;
  void Rescale(const Clock::time_point& when);
};


ExpDecaySample::ExpDecaySample(std::uint32_t reservoirSize, double alpha)
    : impl_ {new ExpDecaySample::Impl {reservoirSize, alpha}} {
}


ExpDecaySample::~ExpDecaySample() {
}


void ExpDecaySample::Clear() {
  impl_->Clear();
}


std::uint64_t ExpDecaySample::size() const {
  return impl_->size();
}


void ExpDecaySample::Update(std::int64_t value) {
  impl_->Update(value);
}


void ExpDecaySample::Update(std::int64_t value, Clock::time_point timestamp) {
  impl_->Update(value, timestamp);
}


Snapshot ExpDecaySample::MakeSnapshot() const {
  return impl_->MakeSnapshot();
}


// === Implementation ===


ExpDecaySample::Impl::Impl(std::uint32_t reservoirSize, double alpha)
    : alpha_         {alpha},
      reservoirSize_ {reservoirSize},
      count_         {},
      rng_           {std::random_device()()},
      dist_          (0, 1) {
    Clear();
}


ExpDecaySample::Impl::~Impl() {
}


void ExpDecaySample::Impl::Clear() {
  std::lock_guard<std::mutex> lock {mutex_};
  values_.clear();
  count_ = 0;
  startTime_ = Clock::now();
  nextScaleTime_ = startTime_ + kRESCALE_THRESHOLD;
}


std::uint64_t ExpDecaySample::Impl::size() const {
  return std::min(reservoirSize_, count_.load());
}


void ExpDecaySample::Impl::Update(std::int64_t value) {
  Update(value, Clock::now());
}


void ExpDecaySample::Impl::Update(std::int64_t value, Clock::time_point timestamp) {
  {
    if (timestamp >= nextScaleTime_) {
      Rescale(timestamp);
    }
    std::lock_guard<std::mutex> lock {mutex_};
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(timestamp - startTime_);
    auto priority = std::exp(alpha_ * dur.count()) / dist_(rng_);
    auto count = ++count_;

    if (count <= reservoirSize_) {
      values_[priority] = value;
    } else {
      auto first = std::begin(values_)->first;
      if (first < priority && values_.insert({priority, value}).second) {
        while (values_.erase(first) == 0) {
          first = std::begin(values_)->first;
        }
      }
    }
  }
}


void ExpDecaySample::Impl::Rescale(const Clock::time_point& when) {
  std::lock_guard<std::mutex> lock {mutex_};
  nextScaleTime_ = when + kRESCALE_THRESHOLD;
  auto oldStartTime = startTime_;
  startTime_ = when;
  size_t size = values_.size();
  std::vector<double> keys;
  std::vector<std::int64_t> values;

  keys.reserve(size);
  values.reserve(size);
  for (auto& kv : values_) {
    keys.push_back(kv.first);
    values.push_back(kv.second);
  }

  values_.clear();

  for (size_t i = 0; i < size; i++) {
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(when - oldStartTime);
    auto key = keys[i] * std::exp(-alpha_ * dur.count());
    values_[key] = values[i];
  }

  count_ = values_.size();
}


Snapshot ExpDecaySample::Impl::MakeSnapshot() const {
  std::vector<double> vals;
  vals.reserve(values_.size());
  for (auto& kv : values_) {
    vals.push_back(kv.second);
  }
  return {vals};
}


} // namespace stats
} // namespace medida
