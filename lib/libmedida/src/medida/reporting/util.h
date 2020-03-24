//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_REPORTING_UTIL_H_
#define MEDIDA_REPORTING_UTIL_H_

#include <chrono>
#include <string>

namespace medida {
namespace reporting {

std::string FormatRateUnit(const std::chrono::nanoseconds& rate_unit);

} // namespace reporting
} // namespace medida

#endif // MEDIDA_REPORTING_UTIL_H_
