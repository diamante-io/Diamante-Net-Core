//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/stats/ewma.h"

#include <gtest/gtest.h>

using namespace medida::stats;

void elapseMinute(EWMA &ewma) {
  for (int i = 1; i <= 12; i++) { // 12 * 5s
    ewma.tick();
  }
}


TEST(EWMATest, aOneMinuteEWMAWithAValueOfThree) {
  auto ewma = EWMA::oneMinuteEWMA();
  ewma.update(3);
  ewma.tick();
  EXPECT_NEAR(0.6, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.22072766, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.08120117, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.02987224, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.01098938, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00404277, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00148725, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00054713, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00020128, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00007405, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00002724, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00001002, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00000369, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00000136, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00000050, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.00000018, ewma.getRate(), 1e-6);
}


TEST(EWMATest, aFiveMinuteEWMAWithAValueOfThree) {
  auto ewma = EWMA::fiveMinuteEWMA();
  ewma.update(3);
  ewma.tick();
  EXPECT_NEAR(0.6, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.49123845, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.40219203, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.32928698, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.26959738, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.22072766, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.18071653, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.14795818, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.12113791, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.09917933, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.08120117, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.06648190, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.05443077, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.04456415, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.03648604, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.02987224, ewma.getRate(), 1e-6);
}


TEST(EWMATest, aFifteenMinuteEWMAWithAValueOfThree) {
  auto ewma = EWMA::fifteenMinuteEWMA();
  ewma.update(3);
  ewma.tick();
  EXPECT_NEAR(0.6, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.56130419, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.52510399, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.49123845, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.45955700, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.42991879, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.40219203, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.37625345, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.35198773, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.32928698, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.30805027, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.28818318, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.26959738, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.25221023, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.23594443, ewma.getRate(), 1e-6);
  elapseMinute(ewma);
  EXPECT_NEAR(0.22072766, ewma.getRate(), 1e-6);
}


TEST(EWMATest, rateDefaultDurationIsOneSecond) {
  auto ewma = EWMA::oneMinuteEWMA();
  ewma.update(3);
  ewma.tick();
  EXPECT_DOUBLE_EQ(ewma.getRate(std::chrono::seconds(1)), ewma.getRate());
  EXPECT_NEAR(0.6, ewma.getRate(), 1e-6);
  EXPECT_NEAR(36.0, ewma.getRate(std::chrono::minutes(1)), 1e-6);
  EXPECT_NEAR(2160.0, ewma.getRate(std::chrono::hours(1)), 1e-6);
}
