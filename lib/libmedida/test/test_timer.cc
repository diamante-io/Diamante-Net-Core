//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/timer.h"

#include <iostream>
#include <thread>

#include <gtest/gtest.h>

#include "medida/metrics_registry.h"

using namespace medida;


struct TimerTest : public ::testing::Test {
  Timer timer;
};


TEST_F(TimerTest, hasDurationUnit) {
  EXPECT_EQ(std::chrono::milliseconds(1), timer.duration_unit());
}


TEST_F(TimerTest, hasRateUnit) {
  EXPECT_EQ(std::chrono::seconds(1), timer.rate_unit());
}


TEST_F(TimerTest, createFromRegistry) {
  MetricsRegistry registry {};
  auto& timer2 = registry.NewTimer({"a", "b", "c"});
  EXPECT_EQ(0, timer2.count());
}


TEST_F(TimerTest, aBlankTimer) {
  EXPECT_EQ(0, timer.count());
  EXPECT_NEAR(0.0, timer.min(), 0.001);
  EXPECT_NEAR(0.0, timer.max(), 0.001);
  EXPECT_NEAR(0.0, timer.mean(), 0.001);
  EXPECT_NEAR(0.0, timer.std_dev(), 0.001);
  EXPECT_NEAR(0.0, timer.mean_rate(), 0.001);
  EXPECT_NEAR(0.0, timer.one_minute_rate(), 0.001);
  EXPECT_NEAR(0.0, timer.five_minute_rate(), 0.001);
  EXPECT_NEAR(0.0, timer.fifteen_minute_rate(), 0.001);

  auto snapshot = timer.GetSnapshot();
  EXPECT_NEAR(0.0, snapshot.getMedian(), 0.001);
  EXPECT_NEAR(0.0, snapshot.get75thPercentile(), 0.001);
  EXPECT_NEAR(0.0, snapshot.get99thPercentile(), 0.001);
  EXPECT_EQ(0, snapshot.size());
}


TEST_F(TimerTest, timingASeriesOfEvents) {
  // Need to sleep between events so sliding window assigns
  // them to separate timeslices.
  timer.Update(std::chrono::milliseconds(10));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  timer.Update(std::chrono::milliseconds(20));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  timer.Update(std::chrono::milliseconds(20));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  timer.Update(std::chrono::milliseconds(30));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  timer.Update(std::chrono::milliseconds(40));

  EXPECT_EQ(5, timer.count());
  EXPECT_NEAR(10.0, timer.min(), 0.001);
  EXPECT_NEAR(40.0, timer.max(), 0.001);
  EXPECT_NEAR(24.0, timer.mean(), 0.001);
  EXPECT_NEAR(11.401, timer.std_dev(), 0.001);

  auto snapshot = timer.GetSnapshot();
  EXPECT_NEAR(20.0, snapshot.getMedian(), 0.001);
  EXPECT_NEAR(30.0, snapshot.get75thPercentile(), 0.001);
  EXPECT_NEAR(39.600000000000001, snapshot.get99thPercentile(), 0.001);
  EXPECT_EQ(5, snapshot.size());
}


TEST_F(TimerTest, timingVariantValues) {
  timer.Update(std::chrono::nanoseconds(9223372036854775807));  // INT64_MAX
  timer.Update(std::chrono::nanoseconds(0));
  EXPECT_NEAR(6.521908912666392E12, timer.std_dev(), 0.001);
}


TEST_F(TimerTest, timerTimeScope) {
  {
    auto t = timer.TimeScope();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  {
    auto t = timer.TimeScope();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  EXPECT_EQ(2, timer.count());
  EXPECT_NEAR(150.0, timer.mean(), 0.5);
}


void my_func() {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}


TEST_F(TimerTest, timerTimeFunction) {
  timer.Time(my_func);
  EXPECT_EQ(1, timer.count());
  EXPECT_NEAR(100.0, timer.mean(), 0.5);
}


TEST_F(TimerTest, timerTimeLambda) {
  timer.Time([]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  });
  EXPECT_EQ(1, timer.count());
  EXPECT_NEAR(100.0, timer.mean(), 1.0);
}
