//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/metrics_registry.h"

#include <gtest/gtest.h>

using namespace medida;

struct MetricsRegistryTest : public ::testing::Test {
  MetricsRegistry registry;
};


TEST_F(MetricsRegistryTest, keysByName) {
  auto& abc = registry.NewCounter({"a", "b", "c"});
  auto& abc2 = registry.NewCounter({"a", "b", "c"});
  auto& abcd = registry.NewCounter({"a", "b", "c", "d"});
  EXPECT_EQ(0, abc.count()) << "Counter a.b.c was not initialied to 0";
  EXPECT_EQ(&abc, &abc2) << "Counter a.b.c was created twice";
  EXPECT_NE(&abc, &abcd) << "Counter a.b.c and a.b.c.d are the same object";
}
