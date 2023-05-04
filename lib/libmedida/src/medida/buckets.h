// Copyright 2020 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#ifndef MEDIDA_BUCKETS_H_
#define MEDIDA_BUCKETS_H_

#include <cstdint>
#include <memory>
#include <set>
#include <map>

#include "medida/metric_interface.h"
#include "medida/timer.h"

namespace medida {

class Buckets : public MetricInterface
{
 public:
    // buckets are derived from `boundaries` as follows:
    // [-INF, b_0), [b_0, b_1), [b_1, b_2), ... , [b_n, + INF]
   Buckets(
       std::set<double> const& boundaries, // expressed in duration_unit
       std::chrono::nanoseconds duration_unit = std::chrono::milliseconds(1),
       std::chrono::nanoseconds rate_unit = std::chrono::seconds(1));
   virtual ~Buckets();

   virtual void Process(MetricProcessor& processor) override;

   std::map<double, std::shared_ptr<Timer>> const& getBuckets();

   std::chrono::nanoseconds boundary_unit() const;
   void Update(std::chrono::nanoseconds value);
   void Clear();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace medida

#endif // MEDIDA_BUCKETS_H_
