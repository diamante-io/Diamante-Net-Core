//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/reporting/abstract_polling_reporter.h"

#include <atomic>
#include <iostream>
#include <thread>

namespace medida {
namespace reporting {

class AbstractPollingReporter::Impl {
 public:
  Impl(AbstractPollingReporter& self);
  ~Impl();
  void Shutdown();
  void Start(Clock::duration period = std::chrono::seconds(5));
 private:
  AbstractPollingReporter& self_;
  std::atomic<bool> running_;
  std::thread thread_;
  void Loop(Clock::duration period);
};


AbstractPollingReporter::AbstractPollingReporter()
    : impl_ {new AbstractPollingReporter::Impl {*this}} {
}


AbstractPollingReporter::~AbstractPollingReporter() {
}


void AbstractPollingReporter::Shutdown() {
  impl_->Shutdown();
}


void AbstractPollingReporter::Start(Clock::duration period) {
  impl_->Start(period);
}


void AbstractPollingReporter::Run() {
}


// === Implementation ===


AbstractPollingReporter::Impl::Impl(AbstractPollingReporter& self)
    : self_ (self),
      running_ {false} {
}


AbstractPollingReporter::Impl::~Impl() {
  Shutdown();
}


void AbstractPollingReporter::Impl::Shutdown() {
  if (running_) {
    running_ = false;
    thread_.join();
  }
}


void AbstractPollingReporter::Impl::Start(Clock::duration period) {
  if (!running_) {
    running_ = true;
    thread_ = std::thread(&AbstractPollingReporter::Impl::Loop, this, period);
  }
}


void AbstractPollingReporter::Impl::Loop(Clock::duration period) {
  while (running_) {
    std::this_thread::sleep_for(period);
    self_.Run();
  }
}


} // namespace reporting
} // namespace medida
