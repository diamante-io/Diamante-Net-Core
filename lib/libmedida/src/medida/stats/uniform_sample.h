//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_UNIFORM_SAMPLE_H_
#define MEDIDA_UNIFORM_SAMPLE_H_

#include <cstdint>
#include <memory>

#include "medida/stats/sample.h"
#include "medida/stats/snapshot.h"

namespace medida {
namespace stats {

class UniformSample : public Sample {
 public:
  UniformSample(std::uint32_t reservoirSize);
  ~UniformSample();
  virtual void Clear();
  virtual std::uint64_t size() const;
  virtual void Update(std::int64_t value);
  virtual Snapshot MakeSnapshot() const;
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace stats
} // namespace medida

#endif // MEDIDA_UNIFORM_SAMPLE_H_
