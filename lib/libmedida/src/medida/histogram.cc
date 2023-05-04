//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/histogram.h"

#include <cmath>
#include <mutex>
#include <algorithm>

#include "medida/stats/exp_decay_sample.h"
#include "medida/stats/uniform_sample.h"
#include "medida/stats/sliding_window_sample.h"

namespace medida {

static const double kDefaultAlpha = 0.015;

// Sliding windows are 5 minutes by default. They also respect the sample-size
// limit by stochastic rate-limiting of additions.
static const std::chrono::seconds kDefaultWindowTime = std::chrono::seconds(5 * 60);

class Histogram::Impl {
 public:
  Impl(SampleType sample_type = kUniform);
  ~Impl();
  stats::Snapshot GetSnapshot() const;
  double sum() const;
  double max() const;
  double min() const;
  double mean() const;
  double std_dev() const;
  void Update(std::int64_t value);
  std::uint64_t count() const;
  double variance() const;
  void Process(MetricProcessor& processor);
  void Clear();
 private:
  static const std::uint64_t kDefaultSampleSize = 1028;
  std::unique_ptr<stats::Sample> sample_;
  double min_;
  double max_;
  double sum_;
  std::uint64_t count_;
  double variance_m_;
  double variance_s_;
  mutable std::mutex mutex_;
};



Histogram::Histogram(SampleType sample_type)
    : impl_ {new Histogram::Impl {sample_type}} {
}


Histogram::~Histogram() {
}


void Histogram::Process(MetricProcessor& processor) {
  processor.Process(*this);  // FIXME: pimpl?
}


void Histogram::Clear() {
  impl_->Clear();
}


std::uint64_t Histogram::count() const {
  return impl_->count();
}


double Histogram::sum() const {
  return impl_->sum();
}


double Histogram::max() const {
  return impl_->max();
}


double Histogram::min() const {
  return impl_->min();
}


double Histogram::mean() const {
  return impl_->mean();
}


double Histogram::std_dev() const {
  return impl_->std_dev();
}


void Histogram::Update(std::int64_t value) {
  impl_->Update(value);
}

stats::Snapshot Histogram::GetSnapshot() const {
  return impl_->GetSnapshot();
}

double Histogram::variance() const {
  return impl_->variance();
}


// === Implementation ===


Histogram::Impl::Impl(SampleType sample_type) {
  if (sample_type == kUniform) {
    sample_ = std::unique_ptr<stats::Sample>(new stats::UniformSample(kDefaultSampleSize));
  } else if (sample_type == kBiased) {
    sample_ = std::unique_ptr<stats::Sample>(new stats::ExpDecaySample(kDefaultSampleSize, kDefaultAlpha));
  } else if (sample_type == kSliding) {
    sample_ = std::unique_ptr<stats::Sample>(new stats::SlidingWindowSample(kDefaultSampleSize,
                                                                            kDefaultWindowTime));
  } else {
      throw std::invalid_argument("invalid sample_type");
  }
  Clear();
}


Histogram::Impl::~Impl() {
}


void Histogram::Impl::Clear() {
  min_ = 0;
  max_ = 0;
  sum_ = 0;
  count_ = 0;
  variance_m_ = 0.0;
  variance_s_ = 0.0;
  sample_->Clear();
}


std::uint64_t Histogram::Impl::count() const {
  std::lock_guard<std::mutex> lock {mutex_};
  return count_;
}


double Histogram::Impl::sum() const {
  std::lock_guard<std::mutex> lock {mutex_};
  return sum_;
}


double Histogram::Impl::max() const {
  std::lock_guard<std::mutex> lock {mutex_};
  if (count_ > 0) {
    return max_;
  }
  return 0.0;
}


double Histogram::Impl::min() const {
  std::lock_guard<std::mutex> lock {mutex_};
  if (count_ > 0) {
    return min_;
  }
  return 0.0;
}


double Histogram::Impl::mean() const {
  std::lock_guard<std::mutex> lock {mutex_};
  if (count_ > 0) {
    return sum_ / (double)count_;
  }
  return 0.0;
}


double Histogram::Impl::std_dev() const {
  double var = variance();
  std::lock_guard<std::mutex> lock {mutex_};
  if (count_ > 0) {
    return std::sqrt(var);
  }
  return 0.0;
}


double Histogram::Impl::variance() const {
  auto c = count();
  if (c > 1) {
    std::lock_guard<std::mutex> lock {mutex_};
    return variance_s_ / (c - 1.0);
  }
  return 0.0;
}


stats::Snapshot Histogram::Impl::GetSnapshot() const {
  return sample_->MakeSnapshot();
}


void Histogram::Impl::Update(std::int64_t value) {
  sample_->Update(value);
  std::lock_guard<std::mutex> lock {mutex_};
  double dval = (double)value;
  if (count_ > 0) {
    max_ = std::max(max_, dval);
    min_ = std::min(min_, dval);
  } else {
    max_ = dval;
    min_ = dval;
  }
  sum_ += dval;
  double new_count = (double)++count_;
  double old_vm = variance_m_;
  double old_vs = variance_s_;
  if (new_count > 1) {
    variance_m_ = old_vm + (dval - old_vm) / new_count;
    variance_s_ = old_vs + (dval - old_vm) * (dval - variance_m_);
  } else {
    variance_m_ = dval;
  }
}


} // namespace medida
