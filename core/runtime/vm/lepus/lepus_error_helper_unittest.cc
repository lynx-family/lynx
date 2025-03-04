// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/lepus_error_helper.h"

#include "core/runtime/vm/lepus/context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#ifdef OS_IOS
#include "trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace lepus {
namespace test {

class LepusErrorHelperTest : public ::testing::Test {
 protected:
  LepusErrorHelperTest() : rt(LEPUS_NewRuntime()), ctx(LEPUS_NewContext(rt)) {}

  ~LepusErrorHelperTest() override {
    LEPUS_FreeContext(ctx);
    LEPUS_FreeRuntime(rt);
  }

  LEPUSRuntime* rt;
  LEPUSContext* ctx;
};

TEST_F(LepusErrorHelperTest, GetErrorStack) {
  if (LEPUS_IsGCMode(ctx)) {
    LEPUS_ThrowTypeError(ctx, "not a function");
    LEPUSValue error = LEPUS_GetException(ctx);
    HandleScope func_scope(ctx, &error, HANDLE_TYPE_LEPUS_VALUE);
    LEPUSValue stack = LEPUS_NewString(ctx, "This is stack");
    func_scope.PushHandle(&stack, HANDLE_TYPE_LEPUS_VALUE);
    LEPUS_SetPropertyStr(ctx, error, "stack", stack);
    EXPECT_EQ(lynx::lepus::LepusErrorHelper::GetErrorStack(ctx, error),
              "This is stack");
  } else {
    LEPUS_ThrowTypeError(ctx, "not a function");
    LEPUSValue error = LEPUS_GetException(ctx);
    LEPUSValue stack = LEPUS_NewString(ctx, "This is stack");
    LEPUS_SetPropertyStr(ctx, error, "stack", stack);
    EXPECT_EQ(lynx::lepus::LepusErrorHelper::GetErrorStack(ctx, error),
              "This is stack");
    LEPUS_FreeValue(ctx, error);
  }
}

TEST_F(LepusErrorHelperTest, GetErrorMessage) {
  if (LEPUS_IsGCMode(ctx)) {
    LEPUS_ThrowTypeError(ctx, "not a function");
    LEPUSValue error = LEPUS_GetException(ctx);
    HandleScope func_scope(ctx, &error, HANDLE_TYPE_LEPUS_VALUE);
    EXPECT_EQ(lynx::lepus::LepusErrorHelper::GetErrorMessage(ctx, error),
              "TypeError: not a function");
  } else {
    LEPUS_ThrowTypeError(ctx, "not a function");
    LEPUSValue error = LEPUS_GetException(ctx);
    EXPECT_EQ(lynx::lepus::LepusErrorHelper::GetErrorMessage(ctx, error),
              "TypeError: not a function");
    LEPUS_FreeValue(ctx, error);
  }
}

}  // namespace test
}  // namespace lepus
}  // namespace lynx
