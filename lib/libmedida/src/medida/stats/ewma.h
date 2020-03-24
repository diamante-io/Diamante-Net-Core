//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_EWMA_H_
#define MEDIDA_EWMA_H_

#include <chrono>
#include <cstdint>
#include <memory>

namespace medida {
namespace stats {

class EWMA {
 public:
  EWMA() = delete;
  EWMA(double alpha, std::chrono::nanoseconds interval);
  EWMA(EWMA &&other);
  ~EWMA();
  static EWMA oneMinuteEWMA();
  static EWMA fiveMinuteEWMA();
  static EWMA fifteenMinuteEWMA();
  void update(std::int64_t n);
  void tick();
  double getRate(std::chrono::nanoseconds duration = std::chrono::seconds {1}) const;
  void clear();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace stats
} // namespace medida

#endif // MEDIDA_EWMA_H_
