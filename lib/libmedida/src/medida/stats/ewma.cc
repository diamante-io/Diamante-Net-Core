//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/stats/ewma.h"

#include <atomic>
#include <cmath>

namespace medida {
namespace stats {

static const int kINTERVAL = 5;
static const double kSECONDS_PER_MINUTE = 60.0;
static const int kONE_MINUTE = 1;
static const int kFIVE_MINUTES = 5;
static const int kFIFTEEN_MINUTES = 15;
static const double kM1_ALPHA = 1 - std::exp(-kINTERVAL / kSECONDS_PER_MINUTE / kONE_MINUTE);
static const double kM5_ALPHA = 1 - std::exp(-kINTERVAL / kSECONDS_PER_MINUTE / kFIVE_MINUTES);
static const double kM15_ALPHA = 1 - std::exp(-kINTERVAL / kSECONDS_PER_MINUTE / kFIFTEEN_MINUTES);

class EWMA::Impl {
 public:
  Impl(double alpha, std::chrono::nanoseconds interval);
  Impl(Impl &other);
  ~Impl();
  void update(std::int64_t n);
  void tick();
  double getRate(std::chrono::nanoseconds duration = std::chrono::seconds {1}) const;
  void clear();
 private:
  volatile bool initialized_;
  volatile double rate_;
  std::atomic<std::int64_t> uncounted_;
  const double alpha_;
  const std::int64_t interval_nanos_;
};


EWMA::EWMA(double alpha, std::chrono::nanoseconds interval)
    : impl_ {new EWMA::Impl {alpha, interval}} {
}


EWMA::EWMA(EWMA &&other) 
    : impl_ {new EWMA::Impl{*other.impl_}} {
}


EWMA::~EWMA() {
}


EWMA EWMA::oneMinuteEWMA() {
  return {kM1_ALPHA, std::chrono::seconds{5}};
}


EWMA EWMA::fiveMinuteEWMA() {
  return {kM5_ALPHA, std::chrono::seconds{5}};
}


EWMA EWMA::fifteenMinuteEWMA() {
  return {kM15_ALPHA, std::chrono::seconds{5}};
}


void EWMA::update(std::int64_t n) {
  impl_->update(n);
}


void EWMA::tick() {
  impl_->tick();
}


double EWMA::getRate(std::chrono::nanoseconds duration) const {
  return impl_->getRate(duration);
}

void EWMA::clear()
{
  impl_->clear();
}

// === Implementation ===


EWMA::Impl::Impl(double alpha, std::chrono::nanoseconds interval)
    : initialized_    {false},
      rate_           {0.0},
      uncounted_      {0},
      alpha_          {alpha},
      interval_nanos_ {interval.count()} {
}


EWMA::Impl::Impl(Impl &other)
    : initialized_    {other.initialized_},
      rate_           {other.rate_},
      uncounted_      {other.uncounted_.load()},
      alpha_          {other.alpha_},
      interval_nanos_ {other.interval_nanos_} {
}


EWMA::Impl::~Impl() {
}


void EWMA::Impl::update(std::int64_t n) {
  uncounted_ += n;
}


void EWMA::Impl::tick() {
  double count = uncounted_.exchange(0);
  auto instantRate = count / interval_nanos_;
  if (initialized_) {
    rate_ += (alpha_ * (instantRate - rate_));
  } else {
    rate_ = instantRate;
    initialized_ = true;
  }
}


double EWMA::Impl::getRate(std::chrono::nanoseconds duration) const {
  return rate_ * duration.count();
}

void EWMA::Impl::clear()
{
  initialized_ = false;
  rate_ = 0.0;
  uncounted_ = 0;
}

} // namespace stats
} // namespace medida
