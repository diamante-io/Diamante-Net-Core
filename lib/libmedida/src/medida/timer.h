//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_TIMER_H_
#define MEDIDA_TIMER_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include "medida/metered_interface.h"
#include "medida/metric_interface.h"
#include "medida/metric_processor.h"
#include "medida/sampling_interface.h"
#include "medida/summarizable_interface.h"
#include "medida/timer_context.h"

namespace medida {

class Timer : public MetricInterface, MeteredInterface, SamplingInterface, SummarizableInterface {
 public:
  Timer(std::chrono::nanoseconds duration_unit = std::chrono::milliseconds(1),
      std::chrono::nanoseconds rate_unit = std::chrono::seconds(1));
  ~Timer();
  void Process(MetricProcessor& processor);
  virtual std::chrono::nanoseconds rate_unit() const;
  virtual std::string event_type() const;
  virtual std::uint64_t count() const;
  virtual double fifteen_minute_rate();
  virtual double five_minute_rate();
  virtual double one_minute_rate();
  virtual double mean_rate();
  virtual stats::Snapshot GetSnapshot() const;
  virtual double max() const;
  virtual double min() const;
  virtual double mean() const;
  virtual double std_dev() const;
  virtual double sum() const;
  std::chrono::nanoseconds duration_unit() const;
  void Clear();
  void Update(std::chrono::nanoseconds duration);
  TimerContext TimeScope();
  void Time(std::function<void()>);
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace medida

#endif // MEDIDA_TIMER_H_
