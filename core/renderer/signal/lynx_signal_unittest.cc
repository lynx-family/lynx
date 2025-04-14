// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/signal/lynx_signal_unittest.h"

#include "core/renderer/signal/computation.h"

namespace lynx {
namespace tasm {
namespace test {

// Not used for now.
const std::tuple<bool> test_params[] = {std::make_tuple(true),
                                        std::make_tuple(false)};

TEST_P(SignalTest, TestGetSignalContext) {
  Signal signal0(&signal_context_, nullptr, lepus::Value(1));
  EXPECT_EQ(signal0.signal_context(), &signal_context_);

  Signal signal1(nullptr, nullptr, lepus::Value(1));
  EXPECT_EQ(signal1.signal_context(), nullptr);
}

TEST_P(SignalTest, TestSetValue) {
  Signal signal0(&signal_context_, nullptr, lepus::Value(1));
  EXPECT_EQ(signal0.signal_context(), &signal_context_);

  signal0.SetValue(lepus::Value(1));
  signal0.SetValue(lepus::Value(2));
  Computation computation0(&signal_context_, nullptr, lepus::Value(),
                           lepus::Value(), true, nullptr);

  signal0.computation_list_.emplace_back(&computation0);
  computation0.PushSignal(&signal0);
  signal0.SetValue(lepus::Value(3));

  Signal signal1(nullptr, nullptr, lepus::Value(1));
  EXPECT_EQ(signal1.signal_context(), nullptr);

  signal1.SetValue(lepus::Value(1));
  signal1.SetValue(lepus::Value(2));

  signal1.computation_list_.emplace_back(&computation0);
  computation0.PushSignal(&signal1);
  signal1.SetValue(lepus::Value(3));
}

TEST_P(SignalTest, TestGetValue) {
  Signal signal0(&signal_context_, nullptr, lepus::Value(1));
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

TEST_P(SignalTest, TestCustomEqual) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let signal_0 = __CreateSignal(0);
    let signal_1 = __CreateSignal(0);

    let res_0 = 0;
    let res_1 = 0;
    let res_2 = 0;
    __CreateScope((s) => {
      scope = s;

      let memo_0 = __CreateMemo((pre) => {
        count = count + 1;
        res_0 = __ReadSignal(signal_1) + 1;
        return res_0;
      }, 0, {
        equals: ()=>{return true;}
      });

      let memo_1 = __CreateMemo((pre) => {
        count = count + 1;
        res_1 = __ReadSignal(signal_0) + __ReadSignal(memo_0) + 1;
        return res_1;
      }, 0, {
        equals: ()=>{return true;}
      });

      let memo_2 = __CreateMemo((pre) => {
        count = count + 1;
        res_2 =
          __ReadSignal(signal_0) + __ReadSignal(memo_0) + __ReadSignal(memo_1) + 1;
        return res_2;
      }, 0, {
        equals: ()=>{return true;}
      });

      __CreateComputation(
        () => {
          count = count + 1;
          return (
            __ReadSignal(memo_0) +
            __ReadSignal(memo_1) +
            __ReadSignal(memo_2) +
            1
          );
        },
        0,
        true
      );
    });
    __RunUpdates(() => {
      __WriteSignal(signal_0, 2);
      __WriteSignal(signal_1, 3);
    });


    )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 7);

  lepus::Value res_0 = GetTopLevelVariableByName("res_0");
  EXPECT_EQ(res_0.Number(), 4);

  lepus::Value res_1 = GetTopLevelVariableByName("res_1");
  EXPECT_EQ(res_1.Number(), 4);

  lepus::Value res_2 = GetTopLevelVariableByName("res_2");
  EXPECT_EQ(res_2.Number(), 8);

  tasm_->Destroy();
}

TEST_P(SignalTest, TestNoSkipCompare) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let signal_0 = __CreateSignal(0);
    let signal_1 = __CreateSignal(0);

    let res_0 = 0;
    let res_1 = 0;
    let res_2 = 0;
    __CreateScope((s) => {
      scope = s;

      let memo_0 = __CreateMemo((pre) => {
        count = count + 1;
        res_0 = __ReadSignal(signal_1) + 1;
        return res_0;
      }, 0);

      let memo_1 = __CreateMemo((pre) => {
        count = count + 1;
        res_1 = __ReadSignal(signal_0) + __ReadSignal(memo_0) + 1;
        return res_1;
      }, 0);

      let memo_2 = __CreateMemo((pre) => {
        count = count + 1;
        res_2 =
          __ReadSignal(signal_0) + __ReadSignal(memo_0) + __ReadSignal(memo_1) + 1;
        return res_2;
      }, 0);

      __CreateComputation(
        () => {
          count = count + 1;
          return (
            __ReadSignal(memo_0) +
            __ReadSignal(memo_1) +
            __ReadSignal(memo_2) +
            1
          );
        },
        0,
        true
      );
    });
    __RunUpdates(() => {
      __WriteSignal(signal_0, 0);
      __WriteSignal(signal_1, 0);
    });


    )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 4);

  lepus::Value res_0 = GetTopLevelVariableByName("res_0");
  EXPECT_EQ(res_0.Number(), 1);

  lepus::Value res_1 = GetTopLevelVariableByName("res_1");
  EXPECT_EQ(res_1.Number(), 2);

  lepus::Value res_2 = GetTopLevelVariableByName("res_2");
  EXPECT_EQ(res_2.Number(), 4);

  tasm_->Destroy();
}

TEST_P(SignalTest, TestSkipCompare) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let signal_0 = __CreateSignal(0);
    let signal_1 = __CreateSignal(0);

    let res_0 = 0;
    let res_1 = 0;
    let res_2 = 0;
    __CreateScope((s) => {
      scope = s;

      let memo_0 = __CreateMemo((pre) => {
        count = count + 1;
        res_0 = __ReadSignal(signal_1) + 1;
        return res_0;
      }, 0);

      let memo_1 = __CreateMemo((pre) => {
        count = count + 1;
        res_1 = __ReadSignal(signal_0) + __ReadSignal(memo_0) + 1;
        return res_1;
      }, 0);

      let memo_2 = __CreateMemo((pre) => {
        count = count + 1;
        res_2 =
          __ReadSignal(signal_0) + __ReadSignal(memo_0) + __ReadSignal(memo_1) + 1;
        return res_2;
      }, 0);

      __CreateComputation(
        () => {
          count = count + 1;
          return (
            __ReadSignal(memo_0) +
            __ReadSignal(memo_1) +
            __ReadSignal(memo_2) +
            1
          );
        },
        0,
        true
      );
    });
    __RunUpdates(() => {
      __WriteSignal(signal_0, 0, {skipCompare: true});
      __WriteSignal(signal_1, 0, {skipCompare: true});
    });


    )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 10);

  lepus::Value res_0 = GetTopLevelVariableByName("res_0");
  EXPECT_EQ(res_0.Number(), 1);

  lepus::Value res_1 = GetTopLevelVariableByName("res_1");
  EXPECT_EQ(res_1.Number(), 2);

  lepus::Value res_2 = GetTopLevelVariableByName("res_2");
  EXPECT_EQ(res_2.Number(), 4);

  tasm_->Destroy();
}

INSTANTIATE_TEST_SUITE_P(SignalTestModule, SignalTest,
                         ::testing::ValuesIn(test_params));

}  // namespace test
}  // namespace tasm
}  // namespace lynx
