//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/metric_name.h"

#include <gtest/gtest.h>

using namespace medida;

struct MetricNameTest : public ::testing::Test {
  MetricNameTest() : name {"domain", "type", "name", "scope"} {};
  MetricName name;
};


TEST_F(MetricNameTest, hasADomain) {
  EXPECT_EQ("domain", name.domain());
}


TEST_F(MetricNameTest, hasAType) {
  EXPECT_EQ("type", name.type());
}


TEST_F(MetricNameTest, hasAName) {
  EXPECT_EQ("name", name.name());
}


TEST_F(MetricNameTest, hasAScope) {
  EXPECT_EQ("scope", name.scope());
  EXPECT_TRUE(name.has_scope());
}


TEST_F(MetricNameTest, isHumanReadable) {
  EXPECT_EQ("domain.type.name.scope", name.ToString());
}


TEST_F(MetricNameTest, hasAWorkingEquals) {
  EXPECT_EQ(name, name);
  EXPECT_EQ(MetricName("domain", "type", "name", "scope"), name);
  EXPECT_NE(MetricName("domain", "type", "name"), name);
}


TEST_F(MetricNameTest, hasAWorkingLessThan) {
  EXPECT_FALSE(MetricName("a", "a", "a") < MetricName("a", "a", "a"));

  EXPECT_FALSE(MetricName("a", "a", "b") < MetricName("a", "a", "a"));
  EXPECT_FALSE(MetricName("a", "b", "a") < MetricName("a", "a", "a"));
  EXPECT_FALSE(MetricName("b", "a", "b") < MetricName("a", "a", "a"));
  EXPECT_FALSE(MetricName("a", "a", "a", "a") < MetricName("a", "a", "a"));

  EXPECT_TRUE(MetricName("a", "a", "a") < MetricName("b", "a", "a"));
  EXPECT_TRUE(MetricName("a", "a", "a") < MetricName("a", "b", "a"));
  EXPECT_TRUE(MetricName("a", "a", "a") < MetricName("a", "a", "b"));
  EXPECT_TRUE(MetricName("a", "a", "a") < MetricName("a", "a", "a", "a"));
}


TEST_F(MetricNameTest, hasAWorkingGreaterThan) {
  EXPECT_FALSE(MetricName("a", "a", "a") > MetricName("a", "a", "a"));

  EXPECT_TRUE(MetricName("a", "a", "b") > MetricName("a", "a", "a"));
  EXPECT_TRUE(MetricName("a", "b", "a") > MetricName("a", "a", "a"));
  EXPECT_TRUE(MetricName("b", "a", "b") > MetricName("a", "a", "a"));
  EXPECT_TRUE(MetricName("a", "a", "a", "a") > MetricName("a", "a", "a"));

  EXPECT_FALSE(MetricName("a", "a", "a") > MetricName("b", "a", "a"));
  EXPECT_FALSE(MetricName("a", "a", "a") > MetricName("a", "b", "a"));
  EXPECT_FALSE(MetricName("a", "a", "a") > MetricName("a", "a", "b"));
  EXPECT_FALSE(MetricName("a", "a", "a") > MetricName("a", "a", "a", "a"));
}
