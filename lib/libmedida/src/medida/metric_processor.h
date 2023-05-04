//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_METRIC_PROCESSOR_H_
#define MEDIDA_METRIC_PROCESSOR_H_

namespace medida {

class Counter;
class Histogram;
class Meter;
class MetricInterface;
class Timer;
class Buckets;

class MetricProcessor {
public:
  virtual ~MetricProcessor();
  virtual void Process(Counter& counter);
  virtual void Process(Histogram& histogram);
  virtual void Process(Meter& meter);
  virtual void Process(Timer& timer);
  virtual void Process(MetricInterface& metric);
  virtual void Process(Buckets& buckets);
};

} // namespace medida

#endif // MEDIDA_METRIC_PROCESSOR_H_
