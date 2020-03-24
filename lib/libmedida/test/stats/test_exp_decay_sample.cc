//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/stats/exp_decay_sample.h"

#include <gtest/gtest.h>

using namespace medida::stats;

TEST(ExpDecaySampleTest, aSampleOf100OutOf1000Elements) {
  ExpDecaySample sample {100, 0.99};
  EXPECT_EQ(0, sample.size());

  auto t = medida::Clock::now();

  for (auto i = 0; i < 1000; i++) {
    sample.Update(i, t);
    t += std::chrono::microseconds(1);
  }
  EXPECT_EQ(100, sample.size());

  auto snapshot = sample.MakeSnapshot();
  EXPECT_EQ(100, snapshot.size());

  for (auto& v : snapshot.getValues()) {
    EXPECT_LT(v, 1000.0);
    EXPECT_GE(v, 0.0);
  }
}


TEST(ExpDecaySampleTest, aSampleOf10OutOf1000Elements) {
  ExpDecaySample sample {100, 0.99};
  EXPECT_EQ(0, sample.size());

  for (auto i = 0; i < 10; i++) {
    sample.Update(i);
  }
  EXPECT_EQ(10, sample.size());

  auto snapshot = sample.MakeSnapshot();
  EXPECT_EQ(10, snapshot.size());

  for (auto& v : snapshot.getValues()) {
    EXPECT_LT(v, 10.0);
    EXPECT_GE(v, 0.0);
  }
}


TEST(ExpDecaySampleTest, aHeavilyBiasedSampleOf100OutOf1000Elements) {
  ExpDecaySample sample {1000, 0.01};

  for (auto i = 0; i < 100; i++) {
    sample.Update(i);
  }
  EXPECT_EQ(100, sample.size());

  auto snapshot = sample.MakeSnapshot();
  EXPECT_EQ(100, snapshot.size());

  for (auto& v : snapshot.getValues()) {
    EXPECT_LT(v, 100.0);
    EXPECT_GE(v, 0.0);
  }
}


TEST(ExpDecaySampleTest, longPeriodsOfInactivityShouldNotCorruptSamplingState) {
  ExpDecaySample sample {10, 0.015};
  auto t = medida::Clock::now();

  for (auto i = 0; i < 1000; i++) {
    sample.Update(1000 + i, t);
    t += std::chrono::milliseconds(100);
  }
  EXPECT_EQ(10, sample.size());

  for (auto& v : sample.MakeSnapshot().getValues()) {
    EXPECT_LT(v, 2000.0);
    EXPECT_GE(v, 1000.0);
  }

  // wait for 15 hours and add another value.
  // this should trigger a rescale. Note that the number of samples will be reduced to 2
  // because of the very small scaling factor that will make all existing priorities equal to
  // zero after rescale.
  t += std::chrono::hours(15);
  sample.Update(2000, t);
  EXPECT_EQ(2, sample.size());

  for (auto& v : sample.MakeSnapshot().getValues()) {
    EXPECT_LT(v, 3000.0);
    EXPECT_GE(v, 1000.0);
  }


  // add 1000 values at a rate of 10 values/second
  for (auto i = 0; i < 1000; i++) {
    sample.Update(3000 + i, t);
    t += std::chrono::milliseconds(100);
  }
  EXPECT_EQ(10, sample.size());

  for (auto& v : sample.MakeSnapshot().getValues()) {
    EXPECT_LE(v, 4000.0);
    EXPECT_GE(v, 3000.0);
  }
}
