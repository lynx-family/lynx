// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/signal/computation_unittest.h"

#include <tuple>

namespace lynx {
namespace tasm {
namespace test {

const std::tuple<bool> test_params[] = {
    std::make_tuple(true),   // ues lepus ng
    std::make_tuple(false),  // use lepus
};

TEST_P(ComputationTest, TestLookUpstreamAndMarkDownStream0) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let signal_0 = __CreateSignal(0);
    let signal_1 = __CreateSignal(1);
    __CreateScope((s) => {
      scope = s;

      let memo_0 = __CreateMemo((pre)=>{
        count = count + 1;
        return __ReadSignal(signal_0) + __ReadSignal(signal_1) + 1;
      }, 0);

      let memo_1 = __CreateMemo((pre)=>{
        count = count + 1;
        return __ReadSignal(signal_0) + __ReadSignal(signal_1) + __ReadSignal(memo_0) + 1;
      }, 0);

      __CreateComputation(()=>{
        count = count + 1;
        return __ReadSignal(signal_0) + __ReadSignal(signal_1) + __ReadSignal(memo_0) + __ReadSignal(memo_1)+ 1;
      }, 0, true);

    });
    )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 3);

  tasm_->Destroy();
}

TEST_P(ComputationTest, TestLookUpstreamAndMarkDownStream1) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let signal_0 = __CreateSignal(0);
    let signal_1 = __CreateSignal(1);
    
    let res_0 = 0;
    let res_1 = 0;
    let res_2 = 0;
    __CreateScope((s) => {
      scope = s;

      let memo_0 = __CreateMemo((pre)=>{
        count = count + 1;
        res_0 = __ReadSignal(signal_0) + __ReadSignal(signal_1) + 1;
        return res_0;
      }, 0);

      let memo_1 = __CreateMemo((pre)=>{
        count = count + 1;
        res_1 = __ReadSignal(signal_0) + __ReadSignal(signal_1) + __ReadSignal(memo_0) + 1;
        return res_1;
      }, 0);

      __CreateComputation(()=>{
        count = count + 1;
        res_2 = __ReadSignal(signal_0) + __ReadSignal(signal_1) + __ReadSignal(memo_0) + __ReadSignal(memo_1)+ 1;
        return res_2;
      }, 0, true);

    });
    __RunUpdates(() => {
      __WriteSignal(signal_0, 2);
      __WriteSignal(signal_1, 3);
    });
    )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 6);

  lepus::Value res_0 = GetTopLevelVariableByName("res_0");
  EXPECT_EQ(res_0.Number(), 6);

  lepus::Value res_1 = GetTopLevelVariableByName("res_1");
  EXPECT_EQ(res_1.Number(), 12);

  lepus::Value res_2 = GetTopLevelVariableByName("res_2");
  EXPECT_EQ(res_2.Number(), 24);

  tasm_->Destroy();
}

TEST_P(ComputationTest, TestLookUpstreamAndMarkDownStream2) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let signal_0 = __CreateSignal(0);

    let res_0 = 0;
    let res_1 = 0;
    let res_2 = 0;
    __CreateScope((s) => {
      scope = s;

      let memo_0 = __CreateMemo((pre) => {
        count = count + 1;
        res_0 = __ReadSignal(signal_0) + 1;
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
            __ReadSignal(signal_0) +
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
    });

    )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 8);

  lepus::Value res_0 = GetTopLevelVariableByName("res_0");
  EXPECT_EQ(res_0.Number(), 3);

  lepus::Value res_1 = GetTopLevelVariableByName("res_1");
  EXPECT_EQ(res_1.Number(), 6);

  lepus::Value res_2 = GetTopLevelVariableByName("res_2");
  EXPECT_EQ(res_2.Number(), 12);

  tasm_->Destroy();
}

TEST_P(ComputationTest, TestLookUpstreamAndMarkDownStream3) {
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
            __ReadSignal(signal_0) +
            __ReadSignal(signal_1) +
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
  EXPECT_EQ(count.Number(), 10);

  lepus::Value res_0 = GetTopLevelVariableByName("res_0");
  EXPECT_EQ(res_0.Number(), 4);

  lepus::Value res_1 = GetTopLevelVariableByName("res_1");
  EXPECT_EQ(res_1.Number(), 7);

  lepus::Value res_2 = GetTopLevelVariableByName("res_2");
  EXPECT_EQ(res_2.Number(), 14);

  tasm_->Destroy();
}

INSTANTIATE_TEST_SUITE_P(ComputationTestModule, ComputationTest,
                         ::testing::ValuesIn(test_params));

}  // namespace test
}  // namespace tasm
}  // namespace lynx
