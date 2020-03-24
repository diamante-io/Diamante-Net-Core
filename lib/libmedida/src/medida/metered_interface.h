//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_METERED_INTERFACE_H_
#define MEDIDA_METERED_INTERFACE_H_

#include <string>

#include "medida/types.h"

namespace medida {

class MeteredInterface {
public:
  virtual ~MeteredInterface() {};
  virtual std::chrono::nanoseconds rate_unit() const = 0;
  virtual std::string event_type() const = 0;
  virtual std::uint64_t count() const = 0;
  virtual double fifteen_minute_rate() = 0;
  virtual double five_minute_rate() = 0;
  virtual double one_minute_rate() = 0;
  virtual double mean_rate() = 0;
};

} // namespace medida

#endif // MEDIDA_METERED_INTERFACE_H_
