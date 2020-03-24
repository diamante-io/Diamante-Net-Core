//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_METER_H_
#define MEDIDA_METER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "medida/stats/ewma.h"
#include "medida/metered_interface.h"
#include "medida/metric_interface.h"
#include "medida/metric_processor.h"
#include "medida/stats/sample.h"

namespace medida {

class Meter : public MetricInterface, MeteredInterface {
 public:
  Meter(std::string event_type, std::chrono::nanoseconds rate_unit = std::chrono::seconds(1));
  ~Meter();
  virtual std::chrono::nanoseconds rate_unit() const;
  virtual std::string event_type() const;
  virtual std::uint64_t count() const;
  virtual double fifteen_minute_rate();
  virtual double five_minute_rate();
  virtual double one_minute_rate();
  virtual double mean_rate();
  void Mark(std::uint64_t n = 1);
  void Clear();
  void Process(MetricProcessor& processor);
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace medida

#endif // MEDIDA_METER_H_
