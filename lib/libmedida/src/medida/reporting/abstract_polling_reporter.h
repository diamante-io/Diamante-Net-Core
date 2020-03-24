//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_REPORTING_ABSTRACT_POLLING_REPORTER_H_
#define MEDIDA_REPORTING_ABSTRACT_POLLING_REPORTER_H_

#include <memory>

#include "medida/types.h"

namespace medida {
namespace reporting {

class AbstractPollingReporter {
 public:
  AbstractPollingReporter();
  virtual ~AbstractPollingReporter();
  virtual void Shutdown();
  virtual void Start(Clock::duration period = std::chrono::seconds(5));
  virtual void Run();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace reporting
} // namespace medida

#endif // MEDIDA_REPORTING_ABSTRACT_POLLING_REPORTER_H_
