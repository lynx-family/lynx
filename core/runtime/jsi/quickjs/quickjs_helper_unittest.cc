// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/quickjs/quickjs_helper.h"

#include "core/runtime/jsi/quickjs/quickjs_exception.h"
#include "core/runtime/jsi/quickjs/quickjs_helper.h"

#if !defined(_MSC_VER)
#include <cxxabi.h>
#endif

#include <iostream>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#ifdef OS_IOS
#include "trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace piper {

using detail::QuickjsHelper;
namespace test {

class QuickJSHelperTest : public ::testing::Test {
 protected:
  QuickJSHelperTest() : rt(LEPUS_NewRuntime()), ctx(LEPUS_NewContext(rt)) {}

  ~QuickJSHelperTest() override {
    LEPUS_FreeContext(ctx);
    LEPUS_FreeRuntime(rt);
  }

  LEPUSRuntime* rt;
  LEPUSContext* ctx;
};

TEST_F(QuickJSHelperTest, GetErrorMessageNormalTestGC) {
  if (LEPUS_IsGCMode(ctx)) {
    LEPUS_ThrowTypeError(ctx, "not a function");
    LEPUSValue error = LEPUS_GetException(ctx);
    HandleScope func_scope(ctx, &error, HANDLE_TYPE_LEPUS_VALUE);
    EXPECT_EQ(QuickjsHelper::getErrorMessage(ctx, error),
              "TypeError: not a function");
  } else {
    LEPUS_ThrowTypeError(ctx, "not a function");
    LEPUSValue error = LEPUS_GetException(ctx);
    EXPECT_EQ(QuickjsHelper::getErrorMessage(ctx, error),
              "TypeError: not a function");
    LEPUS_FreeValue(ctx, error);
  }
}

TEST_F(QuickJSHelperTest, GetErrorMessageFailedTest) {
  if (LEPUS_IsGCMode(ctx)) {
    auto obj = LEPUS_NewObject(ctx);
    HandleScope func_scope(ctx, &obj, HANDLE_TYPE_LEPUS_VALUE);
    EXPECT_EQ(QuickjsHelper::getErrorMessage(ctx, obj), "");

    auto number = LEPUS_NewInt64(ctx, 0xff);  // no need to free this number
    EXPECT_EQ(QuickjsHelper::getErrorMessage(ctx, number), "");
  } else {
    auto obj = LEPUS_NewObject(ctx);
    EXPECT_EQ(QuickjsHelper::getErrorMessage(ctx, obj), "");
    LEPUS_FreeValue(ctx, obj);

    auto number = LEPUS_NewInt64(ctx, 0xff);  // no need to free this number
    EXPECT_EQ(QuickjsHelper::getErrorMessage(ctx, number), "");
  }
}

}  // namespace test
}  // namespace piper
}  // namespace lynx
