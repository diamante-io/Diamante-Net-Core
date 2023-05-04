//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/histogram.h"

#include <gtest/gtest.h>

#include "medida/metrics_registry.h"

using namespace medida;

TEST(HistogramTest, anEmptyHistogram) {
  MetricsRegistry registry {};
  auto& histogram = registry.NewHistogram({"a", "b", "c"});

  EXPECT_EQ(0, histogram.count());
  EXPECT_EQ(0.0, histogram.max());
  EXPECT_EQ(0.0, histogram.min());
  EXPECT_EQ(0.0, histogram.mean());
  EXPECT_EQ(0.0, histogram.std_dev());
  EXPECT_EQ(0.0, histogram.sum());

  auto snapshot = histogram.GetSnapshot();
  EXPECT_EQ(0.0, snapshot.getMedian());
  EXPECT_EQ(0.0, snapshot.get75thPercentile());
  EXPECT_EQ(0.0, snapshot.get99thPercentile());
  EXPECT_EQ(0, snapshot.size());
}


TEST(HistogramTest, aHistogramWith1000Elements) {
  MetricsRegistry registry {};
  auto& histogram = registry.NewHistogram({"a", "b", "c"});

  for (auto i = 1; i <= 1000; i++) {
    histogram.Update(i);
  }

  EXPECT_EQ(1000, histogram.count());
  EXPECT_NEAR(1000.0, histogram.max(), 0.001);
  EXPECT_NEAR(1.0, histogram.min(), 0.001);
  EXPECT_NEAR(500.5, histogram.mean(), 0.001);
  EXPECT_NEAR(288.8194360957494, histogram.std_dev(), 0.001);
  EXPECT_NEAR(500500, histogram.sum(), 0.1);

  auto snapshot = histogram.GetSnapshot();
  EXPECT_NEAR(500.5, snapshot.getMedian(), 0.0001);
  EXPECT_NEAR(750.25, snapshot.get75thPercentile(), 0.0001);
  EXPECT_NEAR(990.00999999999999, snapshot.get99thPercentile(), 0.0001);
  EXPECT_EQ(1000, snapshot.size());
}
