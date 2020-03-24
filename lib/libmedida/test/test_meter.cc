//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/meter.h"

#include <thread>

#include <gtest/gtest.h>

#include "medida/metrics_registry.h"

using namespace medida;


TEST(MeterTest, aBlankMeter) {
  Meter meter {"things"};
  EXPECT_EQ("things", meter.event_type());
  EXPECT_EQ(0, meter.count());
  EXPECT_NEAR(0.0, meter.mean_rate(), 0.001);
}


TEST(MeterTest, createFromRegistry) {
  MetricsRegistry registry {};
  auto& meter = registry.NewMeter({"a", "b", "c"}, "things");
  EXPECT_EQ(0, meter.count());
  EXPECT_EQ("things", meter.event_type());
}


TEST(MeterTest, aMeterWithThreeEvents) {
  Meter meter {"things"};
  meter.Mark(3);
  EXPECT_EQ(3, meter.count());
}


TEST(MeterTest, meterTiming) {
  Meter meter {"things"};
  for (auto i = 0; i < 10; i++) {
    meter.Mark();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  EXPECT_EQ(10, meter.count());
  EXPECT_NEAR(10, meter.mean_rate(), 0.1);
}

