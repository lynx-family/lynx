// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/stopwatch.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

using testing::Return;

namespace clay {
namespace testing {

class MockRefreshRateUpdater : public Stopwatch::RefreshRateUpdater {
 public:
  MOCK_CONST_METHOD0(GetFrameBudget, fml::Milliseconds());
};

TEST(Stopwatch, GetDefaultFrameBudgetTest) {
  fml::Milliseconds frame_budget_60fps = fml::RefreshRateToFrameBudget(60);
  // The default constructor sets the frame_budget to 16.6667 (60 fps).
  FixedRefreshRateStopwatch stopwatch;
  fml::Milliseconds actual_frame_budget = stopwatch.GetFrameBudget();
  EXPECT_EQ(frame_budget_60fps, actual_frame_budget);
}

TEST(Stopwatch, GetOneShotFrameBudgetTest) {
  fml::Milliseconds frame_budget_90fps = fml::RefreshRateToFrameBudget(90);
  FixedRefreshRateStopwatch stopwatch(frame_budget_90fps);
  fml::Milliseconds actual_frame_budget = stopwatch.GetFrameBudget();
  EXPECT_EQ(frame_budget_90fps, actual_frame_budget);
}

TEST(Stopwatch, GetFrameBudgetFromUpdaterTest) {
  MockRefreshRateUpdater updater;
  fml::Milliseconds frame_budget_90fps = fml::RefreshRateToFrameBudget(90);
  EXPECT_CALL(updater, GetFrameBudget())
      .Times(1)
      .WillOnce(Return(frame_budget_90fps));
  Stopwatch stopwatch(updater);
  fml::Milliseconds actual_frame_budget = stopwatch.GetFrameBudget();
  EXPECT_EQ(frame_budget_90fps, actual_frame_budget);
}

}  // namespace testing
}  // namespace clay
