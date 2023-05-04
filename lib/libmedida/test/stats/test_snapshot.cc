//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/stats/snapshot.h"

#include <gtest/gtest.h>

using namespace medida::stats;

struct SnapshotTest : public ::testing::Test {
  SnapshotTest()
      : values {5, 1, 2, 3, 4},
        snapshot {values} {
  };
  const std::vector<double> values;
  const Snapshot snapshot;
};


TEST_F(SnapshotTest, smallQuantilesAreTheFirstValue) {
  EXPECT_DOUBLE_EQ(1, snapshot.getValue(0.0));
}


TEST_F(SnapshotTest, bigQuantilesAreTheLastValue) {
  EXPECT_DOUBLE_EQ(5, snapshot.getValue(1.0));
}


TEST_F(SnapshotTest, hasAMedian) {
  EXPECT_DOUBLE_EQ(3, snapshot.getMedian());
}


TEST_F(SnapshotTest, hasAp75) {
  EXPECT_DOUBLE_EQ(4, snapshot.get75thPercentile());
}


TEST_F(SnapshotTest, hasAp95) {
  EXPECT_DOUBLE_EQ(4.7999999999999998, snapshot.get95thPercentile());
}


TEST_F(SnapshotTest, hasAp98) {
  EXPECT_DOUBLE_EQ(4.9199999999999999, snapshot.get98thPercentile());
}


TEST_F(SnapshotTest, hasAp99) {
  EXPECT_DOUBLE_EQ(4.96, snapshot.get99thPercentile());
}


TEST_F(SnapshotTest, hasAp999) {
  EXPECT_DOUBLE_EQ(4.9960000000000004, snapshot.get999thPercentile());
}


TEST_F(SnapshotTest, hasValues) {
  auto ref = {1, 2, 3, 4, 5};
  EXPECT_TRUE(std::equal(ref.begin(), ref.end(), snapshot.getValues().begin()));
}


TEST_F(SnapshotTest, hasASize) {
  EXPECT_EQ(5, snapshot.size());
}
