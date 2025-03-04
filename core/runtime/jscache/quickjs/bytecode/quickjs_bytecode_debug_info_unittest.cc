// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider.h"
#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider_src.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace quickjs {
namespace testing {

TEST(QuickjsBytecodeProviderDebugInfo, GetQuickjsDebugInfoProvider) {
  auto source = QuickjsBytecodeProvider::FromSource(
      "app-service.js", std::make_shared<StringBuffer>("var v = 0;"));
  {
    auto provider =
        source.Compile(base::Version("2.10"), {.strip_debug_info = true});
    EXPECT_TRUE(provider);
    auto qjs_debug_info = source.GetDebugInfoProvider();
    EXPECT_FALSE(qjs_debug_info);
  }
  {
    auto &info = source.GenerateDebugInfo();
    SetLynxTargetSdkVersion(info.context_, "2.14");
    SetDebugInfoOutside(info.context_, false);
    auto provider2 =
        source.Compile(base::Version("2.14"), {.strip_debug_info = false});
    EXPECT_TRUE(provider2);
    auto qjs_debug_info = source.GetDebugInfoProvider();
    EXPECT_TRUE(qjs_debug_info);
    ASSERT_EQ(LEPUS_VALUE_GET_NORM_TAG(qjs_debug_info->top_level_func_),
              LEPUS_TAG_FUNCTION_BYTECODE);
    ASSERT_EQ((int32_t)GetFunctionDebugId(static_cast<LEPUSFunctionBytecode *>(
                  LEPUS_VALUE_GET_PTR(qjs_debug_info->top_level_func_))) +
                  1,
              0);
  }
  {
    auto &info = source.GenerateDebugInfo();
    SetLynxTargetSdkVersion(info.context_, "2.14");
    SetDebugInfoOutside(info.context_, true);
    auto provider3 =
        source.Compile(base::Version("2.14"), {.strip_debug_info = true});
    EXPECT_TRUE(provider3);
    auto qjs_debug_info = source.GetDebugInfoProvider();
    EXPECT_TRUE(qjs_debug_info);
    ASSERT_EQ(LEPUS_VALUE_GET_NORM_TAG(qjs_debug_info->top_level_func_),
              LEPUS_TAG_FUNCTION_BYTECODE);
    ASSERT_GE((int32_t)GetFunctionDebugId(static_cast<LEPUSFunctionBytecode *>(
                  LEPUS_VALUE_GET_PTR(qjs_debug_info->top_level_func_))) +
                  1,
              0);
  }
}

}  // namespace testing
}  // namespace quickjs
}  // namespace piper
}  // namespace lynx
