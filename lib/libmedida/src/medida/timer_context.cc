//
// Copyright (c) 2012 Daniel Lundin
//

#include <stdexcept>

#include "medida/timer_context.h"

#include "medida/timer.h"
#include "medida/types.h"

namespace medida {

class TimerContext::Impl {
 public:
  Impl(Timer& timer);
  ~Impl();
  void Reset();
  std::chrono::nanoseconds Stop();
 private:
  Clock::time_point start_time_;
  Timer& timer_;
  bool active_;
};


TimerContext::TimerContext(TimerContext&& timer)
    : impl_ {std::move(timer.impl_)} {
}

TimerContext::TimerContext(Timer& timer)
    : impl_ {new TimerContext::Impl {timer}} {
}


TimerContext::~TimerContext() {
}

void TimerContext::checkImpl() const
{
  if (!impl_)
  {
    throw std::runtime_error("Access to moved TimerContext::impl_");
  }
}

void TimerContext::Reset() {
  checkImpl();
  impl_->Reset();
}


std::chrono::nanoseconds TimerContext::Stop() {
  checkImpl();
  return impl_->Stop();
}


// === Implementation ===


TimerContext::Impl::Impl(Timer& timer) 
    : timer_ (timer) {  // FIXME: GCC Bug 50025 - Uniform initialization of reference members broken
  Reset();
}


TimerContext::Impl::~Impl() {
  Stop();
}


void TimerContext::Impl::Reset() {
  start_time_ = Clock::now();
  active_ = true;
}


std::chrono::nanoseconds TimerContext::Impl::Stop() {
  if (active_) {
    auto dur = Clock::now() - start_time_;
    timer_.Update(dur);
    active_ = false;
    return dur;
  }
  return std::chrono::nanoseconds(0);
}


} // namespace medida
