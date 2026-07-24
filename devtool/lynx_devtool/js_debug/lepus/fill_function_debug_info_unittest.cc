// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <string>

#include "core/runtime/lepusng/quickjs_debug_info.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_debugger.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#include "quickjs/include/quickjs.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace debug {
namespace {

class FillFunctionDebugInfoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rt_ = LEPUS_NewRuntime();
    ASSERT_NE(rt_, nullptr);
    ctx_ = LEPUS_NewContext(rt_);
    ASSERT_NE(ctx_, nullptr);
    QJSDebuggerInitialize(ctx_);
  }

  void TearDown() override {
    if (ctx_) {
      QJSDebuggerFree(ctx_);
      LEPUS_FreeContext(ctx_);
    }
    if (rt_) LEPUS_FreeRuntime(rt_);
  }

  // Compile source and return the top-level function (not executed).
  LEPUSValue CompileSource(const char* src) {
    return LEPUS_Eval(ctx_, src, strlen(src), "test.js",
                      LEPUS_EVAL_TYPE_GLOBAL | LEPUS_EVAL_FLAG_COMPILE_ONLY);
  }

  // Build a single-function debug_info JSON document with the given function_id
  // and optional "vd" array.  Caller can mutate the returned doc further.
  rapidjson::Document BuildDebugInfo(uint32_t function_id,
                                     rapidjson::Value* vd_arr = nullptr) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& a = doc.GetAllocator();

    doc.AddMember("function_number", 1, a);

    rapidjson::Value entry(rapidjson::kObjectType);
    entry.AddMember("function_id", function_id, a);
    entry.AddMember("line_number", 1, a);
    entry.AddMember("column_number", static_cast<int64_t>(0), a);
    entry.AddMember("pc2line_len", 0, a);
    if (vd_arr) {
      entry.AddMember("vd", *vd_arr, a);
    }

    rapidjson::Value arr(rapidjson::kArrayType);
    arr.PushBack(std::move(entry), a);
    doc.AddMember("function_info", std::move(arr), a);
    return doc;
  }

  LEPUSRuntime* rt_ = nullptr;
  LEPUSContext* ctx_ = nullptr;
};

// Covers the happy path through the vardef restoration code (lines 134-160).
TEST_F(FillFunctionDebugInfoTest, ValidVarDefs) {
  LEPUSValue top_func = CompileSource(
      "function foo(a) { let x = 1; var y = 2; return a + x + y; }");
  ASSERT_FALSE(LEPUS_IsException(top_func));

  uint32_t func_size = 0;
  LEPUSFunctionBytecode** func_list =
      GetDebuggerAllFunction(ctx_, top_func, &func_size);
  ASSERT_NE(func_list, nullptr);
  ASSERT_GT(func_size, 0u);

  // Find a function with vardefs.
  LEPUSFunctionBytecode* b = nullptr;
  for (uint32_t i = 0; i < func_size; ++i) {
    if (func_list[i] && GetFunctionVarDefCount(func_list[i]) > 0) {
      b = func_list[i];
      break;
    }
  }
  ASSERT_NE(b, nullptr) << "No function with var defs found";

  uint32_t func_id = GetFunctionDebugId(b);
  uint32_t var_count = GetFunctionVarDefCount(b);

  // Construct a "vd" array that mirrors the current vardefs.
  rapidjson::Document tmp;
  auto& a = tmp.GetAllocator();
  rapidjson::Value vd_arr(rapidjson::kArrayType);
  for (uint32_t i = 0; i < var_count; ++i) {
    rapidjson::Value vd(rapidjson::kObjectType);
    const char* nm = GetFunctionVarDefName(ctx_, b, i);
    vd.AddMember("n", rapidjson::Value(nm ? nm : "", a), a);
    if (nm && !LEPUS_IsGCMode(ctx_)) LEPUS_FreeCString(ctx_, nm);
    vd.AddMember("sl", GetFunctionVarDefScopeLevel(b, i), a);
    vd.AddMember("sn", GetFunctionVarDefScopeNext(b, i), a);
    vd.AddMember("f", static_cast<int>(GetFunctionVarDefFlags(b, i)), a);
    vd_arr.PushBack(std::move(vd), a);
  }

  rapidjson::Document doc = BuildDebugInfo(func_id, &vd_arr);

  // Exercises the full vardef parsing loop (lines 134-160).
  // SetFunctionVarDefs no-ops because b already has var_defs, but the
  // deserialization code is fully exercised.
  FillFunctionBytecodeDebugInfo(ctx_, b, doc);

  // Vardefs remain intact.
  EXPECT_EQ(GetFunctionVarDefCount(b), var_count);

  LEPUS_FreeValue(ctx_, top_func);
  if (!LEPUS_IsGCMode(ctx_)) lepus_free(ctx_, func_list);
}

// Covers the early-return on a malformed vardef entry (line 150).
TEST_F(FillFunctionDebugInfoTest, MalformedVarDefEntry) {
  LEPUSValue top_func =
      CompileSource("function bar(x) { let y = 1; return x + y; }");
  ASSERT_FALSE(LEPUS_IsException(top_func));

  uint32_t func_size = 0;
  LEPUSFunctionBytecode** func_list =
      GetDebuggerAllFunction(ctx_, top_func, &func_size);
  ASSERT_NE(func_list, nullptr);

  LEPUSFunctionBytecode* b = nullptr;
  for (uint32_t i = 0; i < func_size; ++i) {
    if (func_list[i]) {
      b = func_list[i];
      break;
    }
  }
  ASSERT_NE(b, nullptr);

  uint32_t func_id = GetFunctionDebugId(b);

  // Build a vd array with an entry missing the "n" (name) field.
  rapidjson::Document tmp;
  auto& a = tmp.GetAllocator();
  rapidjson::Value vd_arr(rapidjson::kArrayType);
  rapidjson::Value vd(rapidjson::kObjectType);
  vd.AddMember("sl", 1, a);
  vd.AddMember("sn", -1, a);
  vd.AddMember("f", 0, a);
  // "n" intentionally omitted
  vd_arr.PushBack(std::move(vd), a);

  rapidjson::Document doc = BuildDebugInfo(func_id, &vd_arr);

  // Should hit the early return at line 150 — no crash.
  FillFunctionBytecodeDebugInfo(ctx_, b, doc);

  LEPUS_FreeValue(ctx_, top_func);
  if (!LEPUS_IsGCMode(ctx_)) lepus_free(ctx_, func_list);
}

// Covers the case where the vd entry is not an object (line 146).
TEST_F(FillFunctionDebugInfoTest, NonObjectVarDefEntry) {
  LEPUSValue top_func = CompileSource("function baz(z) { return z; }");
  ASSERT_FALSE(LEPUS_IsException(top_func));

  uint32_t func_size = 0;
  LEPUSFunctionBytecode** func_list =
      GetDebuggerAllFunction(ctx_, top_func, &func_size);
  ASSERT_NE(func_list, nullptr);

  LEPUSFunctionBytecode* b = nullptr;
  for (uint32_t i = 0; i < func_size; ++i) {
    if (func_list[i]) {
      b = func_list[i];
      break;
    }
  }
  ASSERT_NE(b, nullptr);

  uint32_t func_id = GetFunctionDebugId(b);

  // Build a vd array with a non-object entry (integer instead of object).
  rapidjson::Document tmp;
  auto& a = tmp.GetAllocator();
  rapidjson::Value vd_arr(rapidjson::kArrayType);
  vd_arr.PushBack(42, a);

  rapidjson::Document doc = BuildDebugInfo(func_id, &vd_arr);

  // Should hit the early return at line 146 — no crash.
  FillFunctionBytecodeDebugInfo(ctx_, b, doc);

  LEPUS_FreeValue(ctx_, top_func);
  if (!LEPUS_IsGCMode(ctx_)) lepus_free(ctx_, func_list);
}

TEST_F(FillFunctionDebugInfoTest, CompactSchemaRestoresSourceFromRootSource) {
  const char* root_source =
      "let a = 1;\n"
      "function target() { return a; }\n";
  LEPUSValue top_func = CompileSource(root_source);
  ASSERT_FALSE(LEPUS_IsException(top_func));

  uint32_t func_size = 0;
  LEPUSFunctionBytecode** func_list =
      GetDebuggerAllFunction(ctx_, top_func, &func_size);
  ASSERT_NE(func_list, nullptr);

  LEPUSFunctionBytecode* b = nullptr;
  for (uint32_t i = 0; i < func_size; ++i) {
    if (func_list[i] && GetFunctionDebugId(func_list[i]) != 0) {
      b = func_list[i];
      break;
    }
  }
  ASSERT_NE(b, nullptr);

  const std::string function_source = "function target() { return a; }";
  const auto function_source_offset =
      std::string(root_source).find(function_source);
  ASSERT_NE(function_source_offset, std::string::npos);
  rapidjson::Document doc;
  doc.SetObject();
  auto& a = doc.GetAllocator();
  doc.AddMember("v", 2, a);
  doc.AddMember("fn", 1, a);

  rapidjson::Value entry(rapidjson::kObjectType);
  entry.AddMember("id", GetFunctionDebugId(b), a);
  entry.AddMember("ln", 2, a);
  entry.AddMember("cn", static_cast<int64_t>(0), a);
  entry.AddMember("pll", 0, a);
  rapidjson::Value pc2line_buf(rapidjson::kArrayType);
  entry.AddMember("plb", std::move(pc2line_buf), a);
  entry.AddMember("fsl", static_cast<int32_t>(function_source.length()), a);
  entry.AddMember("fso", static_cast<int32_t>(function_source_offset), a);

  rapidjson::Value arr(rapidjson::kArrayType);
  arr.PushBack(std::move(entry), a);
  doc.AddMember("fi", std::move(arr), a);

  FillFunctionBytecodeDebugInfo(ctx_, b, doc, root_source);

  EXPECT_EQ(GetFunctionDebugSourceLen(ctx_, b),
            static_cast<int32_t>(function_source.length()));
  const char* restored_source = GetFunctionDebugSource(ctx_, b);
  ASSERT_NE(restored_source, nullptr);
  EXPECT_EQ(std::string(restored_source), function_source);
  EXPECT_EQ(GetFunctionDebugSourceOffset(ctx_, b),
            static_cast<int32_t>(function_source_offset));

  LEPUS_FreeValue(ctx_, top_func);
  if (!LEPUS_IsGCMode(ctx_)) lepus_free(ctx_, func_list);
}

}  // namespace
}  // namespace debug
}  // namespace lynx
