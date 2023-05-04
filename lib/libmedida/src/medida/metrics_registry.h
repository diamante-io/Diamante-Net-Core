//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_METRICS_REGISTRY_H_
#define MEDIDA_METRICS_REGISTRY_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "medida/counter.h"
#include "medida/histogram.h"
#include "medida/meter.h"
#include "medida/metric_interface.h"
#include "medida/metric_name.h"
#include "medida/metric_processor.h"
#include "medida/timer.h"
#include "medida/buckets.h"

namespace medida {

class MetricsRegistry {
 public:
  MetricsRegistry();
  ~MetricsRegistry();
  Counter& NewCounter(const MetricName &name, std::int64_t init_value = 0);
  Histogram& NewHistogram(const MetricName &name,
      SamplingInterface::SampleType sample_type = SamplingInterface::kUniform);
  Meter& NewMeter(const MetricName &name, std::string event_type, 
      Clock::duration rate_unit = std::chrono::seconds(1));
  Timer& NewTimer(const MetricName &name,
      std::chrono::nanoseconds duration_unit = std::chrono::milliseconds(1),
      std::chrono::nanoseconds rate_unit = std::chrono::seconds(1));
  Buckets& NewBuckets(
      const MetricName& name,
      std::set<double> boundaries,
      std::chrono::nanoseconds duration_unit = std::chrono::milliseconds(1),
      std::chrono::nanoseconds rate_unit = std::chrono::seconds(1));

  std::map<MetricName, std::shared_ptr<MetricInterface>> GetAllMetrics() const;
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace medida

#endif // MEDIDA_METRICS_REGISTRY_H_
