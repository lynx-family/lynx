// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepusng/quick_context.h"

#include <cstdint>
#include <string>

#include "core/runtime/lepus/bytecode_generator.h"
#include "quickjs/include/quickjs.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {
namespace {

void ExpectIntResult(QuickContext* context, const std::string& function_name,
                     int32_t expected) {
  LEPUSValue result = context->GetAndCall(function_name, nullptr, 0);
  ASSERT_EQ(LEPUS_TAG_INT, LEPUS_VALUE_GET_TAG(result));
  EXPECT_EQ(expected, LEPUS_VALUE_GET_INT(result));
  if (!context->GetGCFlag()) {
    LEPUS_FreeValue(context->context(), result);
  }
}

TEST(QuickContextSourceBundleTest, DeserializesAndExecutesSource) {
  QuickContextBundle bundle;
  bundle.SetSource("function sourceResult() { return 42; }");

  QuickContext context;
  ASSERT_TRUE(
      context.DeSerialize(bundle, false, nullptr, "main-thread-test.js"));
  EXPECT_EQ(bundle.source(), context.GetDebugSourceCode());
  ASSERT_TRUE(context.Execute());
  ExpectIntResult(&context, "sourceResult", 42);
}

TEST(QuickContextSourceBundleTest, DeserializesSourceIntoReusedContext) {
  QuickContextBundle initial_bundle;
  initial_bundle.SetSource("function initialResult() { return 1; }");

  QuickContext context;
  ASSERT_TRUE(
      context.DeSerialize(initial_bundle, false, nullptr, "main-thread.js"));
  ASSERT_TRUE(context.Execute());

  QuickContextBundle reused_bundle;
  reused_bundle.SetSource("function reusedResult() { return 2; }");
  Value eval_result;
  ASSERT_TRUE(context.DeSerialize(reused_bundle, true, &eval_result,
                                  "dynamic-component.js"));

  ExpectIntResult(&context, "initialResult", 1);
  ExpectIntResult(&context, "reusedResult", 2);
}

TEST(QuickContextSourceBundleTest, RejectsInvalidSource) {
  QuickContextBundle bundle;
  bundle.SetSource("function invalid(");

  QuickContext context;
  EXPECT_FALSE(context.DeSerialize(bundle, false, nullptr, "invalid.js"));
}

TEST(QuickContextSourceBundleTest, KeepsBytecodeDeserializationWorking) {
  constexpr char kSource[] = "function bytecodeResult() { return 7; }";
  QuickContext compiler_context;
  ASSERT_TRUE(
      BytecodeGenerator::GenerateBytecode(&compiler_context, kSource, "2.0")
          .empty());

  size_t bytecode_size = 0;
  uint8_t* bytecode = LEPUS_WriteObject(
      compiler_context.context(), &bytecode_size,
      compiler_context.GetTopLevelFunction(), LEPUS_WRITE_OBJ_BYTECODE);
  ASSERT_NE(nullptr, bytecode);

  QuickContextBundle bundle;
  bundle.lepus_code().assign(bytecode, bytecode + bytecode_size);
  bundle.lepusng_code_len() = bytecode_size;
  if (!compiler_context.GetGCFlag()) {
    lepus_free(compiler_context.context(), bytecode);
  }

  QuickContext context;
  ASSERT_TRUE(context.DeSerialize(bundle, false, nullptr, "bytecode.lepus"));
  ASSERT_TRUE(context.Execute());
  ExpectIntResult(&context, "bytecodeResult", 7);
}

}  // namespace
}  // namespace lepus
}  // namespace lynx
