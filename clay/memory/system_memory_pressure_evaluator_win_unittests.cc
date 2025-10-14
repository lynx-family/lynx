// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/memory/system_memory_pressure_evaluator_win.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
class MockSystemMemoryPressureEvaluatorWin
    : public SystemMemoryPressureEvaluatorWin {
 public:
  MockSystemMemoryPressureEvaluatorWin() : SystemMemoryPressureEvaluatorWin() {}
  void SetSupportOsSignal(bool support) { support_os_signal_ = support; }
  MOCK_METHOD(void, PeriodicallyCheck, (), (override));

 private:
  bool TryOsSignalObserving() override { return support_os_signal_; }

  bool support_os_signal_ = false;
};

TEST(SystemMemoryPressureEvaluatorWinTest, TestPolling) {
  {
    auto evaluator = std::make_unique<MockSystemMemoryPressureEvaluatorWin>();
    // Both default values are set to false.
    EXPECT_FALSE(evaluator->EnablePolling());
    EXPECT_FALSE(evaluator->enable_os_signal_);

    EXPECT_CALL(*evaluator, PeriodicallyCheck()).Times(1);
    evaluator->StartObserving();
    EXPECT_TRUE(evaluator->EnablePolling());
    EXPECT_FALSE(evaluator->enable_os_signal_);
  }

  {
    auto evaluator = std::make_unique<MockSystemMemoryPressureEvaluatorWin>();
    evaluator->SetSupportOsSignal(true);

    EXPECT_CALL(*evaluator, PeriodicallyCheck()).Times(0);
    evaluator->StartObserving();
    EXPECT_FALSE(evaluator->EnablePolling());
    EXPECT_TRUE(evaluator->enable_os_signal_);
  }
}
}  // namespace clay
