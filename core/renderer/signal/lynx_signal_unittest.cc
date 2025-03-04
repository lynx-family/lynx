// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/signal/lynx_signal_unittest.h"

#include "core/renderer/signal/computation.h"

namespace lynx {
namespace tasm {
namespace testing {

void SignalTest::SetUp() {}

void SignalTest::TearDown() {}

// Not used for now.
const std::tuple<bool> test_params[] = {std::make_tuple(true),
                                        std::make_tuple(false)};

TEST_P(SignalTest, TestGetSignalContext) {
  Signal signal0(&signal_context_, lepus::Value(1));
  EXPECT_EQ(signal0.signal_context(), &signal_context_);

  Signal signal1(nullptr, lepus::Value(1));
  EXPECT_EQ(signal1.signal_context(), nullptr);
}

TEST_P(SignalTest, TestSetValue) {
  Signal signal0(&signal_context_, lepus::Value(1));
  EXPECT_EQ(signal0.signal_context(), &signal_context_);

  signal0.SetValue(lepus::Value(1));
  signal0.SetValue(lepus::Value(2));
  Computation computation0(&signal_context_, nullptr, lepus::Value(),
                           lepus::Value(), true, nullptr);

  signal0.computation_list_.emplace_back(&computation0);
  signal0.SetValue(lepus::Value(3));

  Signal signal1(nullptr, lepus::Value(1));
  EXPECT_EQ(signal1.signal_context(), nullptr);

  signal1.SetValue(lepus::Value(1));
  signal1.SetValue(lepus::Value(2));

  signal1.computation_list_.emplace_back(&computation0);
  signal1.SetValue(lepus::Value(3));
}

TEST_P(SignalTest, TestGetValue) {
  Signal signal0(&signal_context_, lepus::Value(1));
  EXPECT_EQ(signal0.signal_context(), &signal_context_);

  EXPECT_EQ(signal0.GetValue(), lepus::Value(1));
  EXPECT_EQ(signal0.computation_list_.size(), 0);

  Computation computation0(&signal_context_, nullptr, lepus::Value(),
                           lepus::Value(), true, nullptr);
  signal_context_.computation_stack_.emplace_back(&computation0);

  EXPECT_EQ(signal0.GetValue(), lepus::Value(1));
  EXPECT_EQ(signal0.computation_list_.size(), 1);
  EXPECT_EQ(signal0.computation_list_.front(), &computation0);
}

INSTANTIATE_TEST_SUITE_P(SignalTestModule, SignalTest,
                         ::testing::ValuesIn(test_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
