//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/reporting/collectd_reporter.h"

#include <thread>

#include <gtest/gtest.h>

#include "medida/metrics_registry.h"

using namespace medida;
using namespace medida::reporting;


TEST(CollectdReporterTest, foo) {
  MetricsRegistry registry {};
  auto& counter = registry.NewCounter({"test", "console_reporter", "mycounter"});
  auto& histogram = registry.NewHistogram({"test", "console_reporter", "myhistogram"});
  auto& meter = registry.NewMeter({"test", "console_reporter", "mymeter"}, "cycles");
  auto& timer = registry.NewTimer({"test", "console_reporter", "mytimer"});
  CollectdReporter reporter {registry, "localhost", 25826};
  for (auto i = 1; i <= 100; i++) {
    auto t = timer.TimeScope();
    counter.inc();
    histogram.Update(i);
    meter.Mark();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  reporter.Run();
}


