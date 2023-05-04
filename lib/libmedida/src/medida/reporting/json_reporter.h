//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_REPORTING_JSON_REPORTER_H_
#define MEDIDA_REPORTING_JSON_REPORTER_H_

#include <memory>
#include <string>

#include "medida/metric_processor.h"
#include "medida/metrics_registry.h"

namespace medida {
namespace reporting {

class JsonReporter : public MetricProcessor {
 public:
  JsonReporter(MetricsRegistry &registry);
  virtual ~JsonReporter();
  virtual void Process(Counter& counter);
  virtual void Process(Meter& meter);
  virtual void Process(Histogram& histogram);
  virtual void Process(Timer& timer);
  virtual void Process(Buckets& buckets);
  virtual std::string Report();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};


} // namespace reporting
} // namespace medida

#endif // MEDIDA_REPORTING_JSON_REPORTER_H_
