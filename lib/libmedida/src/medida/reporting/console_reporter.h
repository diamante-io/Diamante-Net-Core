//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_REPORTING_CONSOLE_REPORTER_H_
#define MEDIDA_REPORTING_CONSOLE_REPORTER_H_

#include <iostream>

#include "medida/metric_processor.h"
#include "medida/metrics_registry.h"
#include "medida/reporting/abstract_polling_reporter.h"

namespace medida {
namespace reporting {

class ConsoleReporter : public AbstractPollingReporter, public MetricProcessor {
 public:
  ConsoleReporter(MetricsRegistry &registry, std::ostream& out = std::cerr);
  virtual ~ConsoleReporter();
  virtual void Run();
  virtual void Process(Counter& counter);
  virtual void Process(Meter& meter);
  virtual void Process(Histogram& histogram);
  virtual void Process(Timer& timer);
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};


} // namespace reporting
} // namespace medida

#endif // MEDIDA_REPORTING_CONSOLE_REPORTER_H_
