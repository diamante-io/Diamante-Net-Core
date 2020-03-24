//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/counter.h"

#include <gtest/gtest.h>

using namespace medida;

struct CounterTest : public ::testing::Test {
  Counter counter;
};


TEST_F(CounterTest, startsAtZero) {
  EXPECT_EQ(0, counter.count());
}


TEST_F(CounterTest, initValue) {
  Counter c = {42};
  EXPECT_EQ(42, c.count());
}


TEST_F(CounterTest, incrementsByOne) {
  counter.inc();
  EXPECT_EQ(1, counter.count());
}


TEST_F(CounterTest, incrementsByAnArbitraryDelta) {
  counter.inc(42);
  EXPECT_EQ(42, counter.count());
}


TEST_F(CounterTest, incrementsByAnArbitraryNegativeDelta) {
  counter.inc(-42);
  EXPECT_EQ(-42, counter.count());
}


TEST_F(CounterTest, decrementsByOne) {
  counter.dec();
  EXPECT_EQ(-1, counter.count());
}


TEST_F(CounterTest, decrementsByAnArbitraryDelta) {
  counter.dec(42);
  EXPECT_EQ(-42, counter.count());
}


TEST_F(CounterTest, decrementsByAnArbitraryNegativeDelta) {
  counter.dec(-42);
  EXPECT_EQ(42, counter.count());
}


TEST_F(CounterTest, isZeroAfterBeingCleared) {
  counter.dec(4);
  counter.clear();
  EXPECT_EQ(0, counter.count());
}
