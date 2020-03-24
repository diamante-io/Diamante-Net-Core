//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/reporting/json_reporter.h"

#include <gtest/gtest.h>

#include "medida/metrics_registry.h"

using namespace medida;
using namespace medida::reporting;


TEST(JsonReporterTest, foo) {
  MetricsRegistry registry {};
  auto& counter = registry.NewCounter({"test", "console_reporter", "counter"});
  auto& histogram = registry.NewHistogram({"test", "console_reporter", "histogram"});
  auto& meter = registry.NewMeter({"test", "console_reporter", "meter"}, "cycles");
  auto& timer = registry.NewTimer({"test", "console_reporter", "timer"});
  JsonReporter reporter {registry};
  counter.inc();
  for (auto i = 1; i <= 1000; i++) {
    auto t = timer.TimeScope();
    histogram.Update(i);
    meter.Mark();
  }
  auto json = reporter.Report();
  std::cerr << json << std::endl;
}


