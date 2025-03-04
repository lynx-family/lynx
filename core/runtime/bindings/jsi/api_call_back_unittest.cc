// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/api_call_back.h"

#include "core/runtime/jsi/jsi_unittest.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {

class ApiCallBackTest : public JSITestBase {
 protected:
  ApiCallBackManager manager_;
};

TEST_P(ApiCallBackTest, CreateAndCallTest) {
  piper::Function func = function("function (i) { globalThis.result = i; }");
  ApiCallBack callback = manager_.createCallbackImpl(std::move(func));

  EXPECT_TRUE(callback.IsValid());
  manager_.InvokeWithValue(&rt, callback, 42);

  EXPECT_EQ(eval("globalThis.result")->getNumber(), 42);
}

TEST_P(ApiCallBackTest, CreateAndCallWithNonArgsTest) {
  piper::Function func = function("function (i) { globalThis.result = i; }");
  ApiCallBack callback = manager_.createCallbackImpl(std::move(func));

  EXPECT_TRUE(callback.IsValid());
  manager_.InvokeWithValue(&rt, callback);

  EXPECT_TRUE(eval("globalThis.result")->isUndefined());
}

TEST_P(ApiCallBackTest, CreateAndCallMultipleArgsTest) {
  piper::Function func =
      function("function (i, j, k) { globalThis.result = i + j + k; }");
  ApiCallBack callback = manager_.createCallbackImpl(std::move(func));

  manager_.InvokeWithValue(&rt, callback, 42, 24, 69);
  EXPECT_EQ(eval("globalThis.result")->getNumber(), 42 + 24 + 69);
}

TEST_P(ApiCallBackTest, CallNonExistTest) {
  manager_.InvokeWithValue(&rt, ApiCallBack());  // should not crash

  piper::Function func = function("function (i) { globalThis.result = i; }");
  ApiCallBack callback = manager_.createCallbackImpl(std::move(func));

  EXPECT_TRUE(callback.IsValid());
  manager_.InvokeWithValue(&rt, callback, 42);

  manager_.InvokeWithValue(&rt, callback);  // this should has no effect

  EXPECT_FALSE(eval("globalThis.result")->isUndefined());

  callback = manager_.createCallbackImpl(
      function("function () { globalThis.result = 'foo'; }"));
  manager_.Destroy();

  manager_.InvokeWithValue(&rt, callback);
  EXPECT_FALSE(eval("globalThis.result")->isString());
}

TEST_P(ApiCallBackTest, CallWithLepusValueTest) {
  piper::Function func =
      function("function (i) { globalThis.result = JSON.stringify(i); }");
  ApiCallBack callback = manager_.createCallbackImpl(std::move(func));

  const lepus::Value value =
      lepus::jsonValueTolepusValue(R"({ "foo": "bar" })");
  manager_.InvokeWithValue(&rt, callback, value);

  EXPECT_EQ(eval("globalThis.result")->getString(rt).utf8(rt),
            R"({"foo":"bar"})");

  piper::Function func2 = function("function (i) { globalThis.result = i; }");

  callback = manager_.createCallbackImpl(std::move(func2));

  // Call with null
  const lepus::Value nil_value = lepus::Value();
  manager_.InvokeWithValue(&rt, callback, nil_value);
  EXPECT_TRUE(eval("globalThis.result")->isNull());
}

INSTANTIATE_TEST_SUITE_P(
    Runtimes, ApiCallBackTest, ::testing::ValuesIn(runtimeGenerators()),
    [](const ::testing::TestParamInfo<ApiCallBackTest::ParamType>& info) {
      auto rt = info.param(nullptr);
      switch (rt->type()) {
        case JSRuntimeType::v8:
          return "v8";
        case JSRuntimeType::jsc:
          return "jsc";
        case JSRuntimeType::quickjs:
          return "quickjs";
      }
    });

}  // namespace test
}  // namespace piper
}  // namespace lynx
