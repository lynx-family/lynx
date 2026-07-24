// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepusng/quickjs_debug_info.h"

#include <string>
#include <vector>

#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/template_bundle/template_codec/binary_encoder/encode_util.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#include "third_party/rapidjson/document.h"

extern "C" {
#include "quickjs/include/quickjs.h"
}

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {

class QuickjsDebugInfoVarDefTest : public ::testing::Test {
 protected:
  QuickjsDebugInfoVarDefTest() = default;
  ~QuickjsDebugInfoVarDefTest() override = default;

  // Compile source with debuginfo_outside enabled and return the debug JSON.
  std::string BuildDebugJson(const std::string& src, bool var_defs_outside) {
    QuickContext qctx;
    qctx.set_debuginfo_outside(true);
    std::string err = BytecodeGenerator::GenerateBytecode(&qctx, src, "4.1");
    EXPECT_TRUE(err.empty()) << "Compile error: " << err;

    return QuickjsDebugInfoBuilder::BuildJsDebugInfo(
        qctx.context(), qctx.GetTopLevelFunction(), src,
        /*debuginfo_outside=*/true, var_defs_outside);
  }

  rapidjson::Document BuildDebugInfoThroughBuilder(const std::string& src,
                                                   const std::string& sdk) {
    QuickContext qctx;
    qctx.set_debuginfo_outside(true);
    std::string err = BytecodeGenerator::GenerateBytecode(&qctx, src, sdk);
    EXPECT_TRUE(err.empty()) << "Compile error: " << err;

    tasm::LepusDebugInfo info;
    info.debug_info_.source_code = src;
    info.debug_info_.top_level_function = qctx.GetTopLevelFunction();

    QuickjsDebugInfoBuilder builder;
    builder.AddDebugInfo("lepus.js", info, &qctx);

    rapidjson::Document doc;
    auto value = builder.TakeDebugInfo();
    doc.CopyFrom(value, doc.GetAllocator());
    return doc;
  }
};

// Test: var_defs_outside=true produces vardef entries with correct keys.
TEST_F(QuickjsDebugInfoVarDefTest, VarInfoOutsideTrue_ProducesVarDefEntries) {
  std::string src =
      "function foo(a) {\n"
      "  let x = 1;\n"
      "  var y = 2;\n"
      "  return a + x + y;\n"
      "}\n";

  std::string json_str = BuildDebugJson(src, /*var_defs_outside=*/true);

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("function_info"));
  ASSERT_TRUE(doc["function_info"].IsArray());
  ASSERT_GT(doc["function_info"].Size(), 0u);

  // Find a function with "vd" key
  bool found_var_defs = false;
  for (rapidjson::SizeType i = 0; i < doc["function_info"].Size(); ++i) {
    const auto& fi = doc["function_info"][i];
    if (!fi.HasMember("vd")) continue;
    found_var_defs = true;

    ASSERT_TRUE(fi["vd"].IsArray());
    ASSERT_GT(fi["vd"].Size(), 0u);

    // Each entry must have all 4 required keys with correct types
    for (rapidjson::SizeType j = 0; j < fi["vd"].Size(); ++j) {
      const auto& vd = fi["vd"][j];
      EXPECT_TRUE(vd.HasMember("n")) << "Missing 'n' at index " << j;
      EXPECT_TRUE(vd.HasMember("sl")) << "Missing 'sl' at index " << j;
      EXPECT_TRUE(vd.HasMember("sn")) << "Missing 'sn' at index " << j;
      EXPECT_TRUE(vd.HasMember("f")) << "Missing 'f' at index " << j;
      EXPECT_TRUE(vd["n"].IsString());
      EXPECT_TRUE(vd["sl"].IsInt());
      EXPECT_TRUE(vd["sn"].IsInt());
      EXPECT_TRUE(vd["f"].IsInt());
    }
  }
  EXPECT_TRUE(found_var_defs) << "No function had 'vd' entries";
}

// Test: var_defs_outside=false does NOT produce vardef entries.
TEST_F(QuickjsDebugInfoVarDefTest, VarInfoOutsideFalse_NoVarDefEntries) {
  std::string src =
      "function foo(a) {\n"
      "  let x = 1;\n"
      "  return a + x;\n"
      "}\n";

  std::string json_str = BuildDebugJson(src, /*var_defs_outside=*/false);

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("function_info"));

  for (rapidjson::SizeType i = 0; i < doc["function_info"].Size(); ++i) {
    EXPECT_FALSE(doc["function_info"][i].HasMember("vd"))
        << "Function at index " << i << " should not have var_defs";
  }
}

// Test: multiple functions each have structurally valid var_defs.
TEST_F(QuickjsDebugInfoVarDefTest, MultipleFunctions_EachHasCorrectVarDefs) {
  std::string src =
      "let a = 1;\n"
      "function foo(x) {\n"
      "  let b = 2;\n"
      "  return x + b;\n"
      "}\n"
      "function bar(y, z) {\n"
      "  var c = 3;\n"
      "  var d = 4;\n"
      "  return y + z + c + d;\n"
      "}\n";

  std::string json_str = BuildDebugJson(src, /*var_defs_outside=*/true);

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("function_info"));
  ASSERT_GE(doc["function_info"].Size(), 3u);  // top-level + foo + bar

  uint32_t funcs_with_var_defs = 0;
  for (rapidjson::SizeType i = 0; i < doc["function_info"].Size(); ++i) {
    const auto& fi = doc["function_info"][i];
    if (!fi.HasMember("vd")) continue;
    funcs_with_var_defs++;

    const auto& vd_arr = fi["vd"];
    ASSERT_TRUE(vd_arr.IsArray());
    for (rapidjson::SizeType j = 0; j < vd_arr.Size(); ++j) {
      EXPECT_TRUE(vd_arr[j].HasMember("n"));
      EXPECT_TRUE(vd_arr[j].HasMember("sl"));
      EXPECT_TRUE(vd_arr[j].HasMember("sn"));
      EXPECT_TRUE(vd_arr[j].HasMember("f"));
    }
  }
  EXPECT_GE(funcs_with_var_defs, 2u)
      << "Expected at least 2 functions with var_defs (foo and bar)";
}

// Test: JSON vardef values match C API getter results on the same bytecodes.
TEST_F(QuickjsDebugInfoVarDefTest, VarDefValuesMatchBytecodeGetters) {
  QuickContext qctx;
  qctx.set_debuginfo_outside(true);
  std::string src =
      "let x = 1;\n"
      "function foo(a) {\n"
      "  let y = 2;\n"
      "  var z = 3;\n"
      "  return a + y + z;\n"
      "}\n";
  std::string err = BytecodeGenerator::GenerateBytecode(&qctx, src, "4.1");
  ASSERT_TRUE(err.empty()) << "Compile error: " << err;

  LEPUSContext* ctx = qctx.context();
  LEPUSValue top_func = qctx.GetTopLevelFunction();

  // Get function list via C API
  uint32_t func_size = 0;
  LEPUSFunctionBytecode** func_list =
      GetDebuggerAllFunction(ctx, top_func, &func_size);
  ASSERT_NE(func_list, nullptr);
  ASSERT_GT(func_size, 0u);

  // Serialize
  std::string json_str = QuickjsDebugInfoBuilder::BuildJsDebugInfo(
      ctx, top_func, src, /*debuginfo_outside=*/true,
      /*var_defs_outside=*/true);

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("function_info"));
  ASSERT_EQ(doc["function_info"].Size(), func_size);

  // Compare JSON values against C API getters for each function
  for (uint32_t fi = 0; fi < func_size; ++fi) {
    LEPUSFunctionBytecode* b = func_list[fi];
    if (!b) continue;

    uint32_t count = GetFunctionVarDefCount(b);
    const auto& func_json = doc["function_info"][fi];

    if (count == 0) {
      EXPECT_FALSE(func_json.HasMember("vd"));
      continue;
    }

    ASSERT_TRUE(func_json.HasMember("vd"));
    ASSERT_EQ(func_json["vd"].Size(), count);

    for (uint32_t i = 0; i < count; ++i) {
      const char* nm = GetFunctionVarDefName(ctx, b, i);
      std::string expected_name = nm ? nm : "";
      if (nm && !LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, nm);

      EXPECT_EQ(std::string(func_json["vd"][i]["n"].GetString()), expected_name)
          << "Name mismatch at func " << fi << " var " << i;
      EXPECT_EQ(func_json["vd"][i]["sl"].GetInt(),
                GetFunctionVarDefScopeLevel(b, i))
          << "Scope level mismatch at func " << fi << " var " << i;
      EXPECT_EQ(func_json["vd"][i]["sn"].GetInt(),
                GetFunctionVarDefScopeNext(b, i))
          << "Scope next mismatch at func " << fi << " var " << i;
      EXPECT_EQ(func_json["vd"][i]["f"].GetInt(),
                static_cast<int>(GetFunctionVarDefFlags(b, i)))
          << "Flags mismatch at func " << fi << " var " << i;
    }
  }

  if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, func_list);
}

// Test: round-trip serialize → parse JSON → SetFunctionVarDefs → verify match.
TEST_F(QuickjsDebugInfoVarDefTest, RoundTrip_SerializeAndRestoreVarDefs) {
  QuickContext qctx;
  qctx.set_debuginfo_outside(true);
  std::string src =
      "function bar(a, b) {\n"
      "  let sum = a + b;\n"
      "  var diff = a - b;\n"
      "  return sum + diff;\n"
      "}\n";
  std::string err = BytecodeGenerator::GenerateBytecode(&qctx, src, "4.1");
  ASSERT_TRUE(err.empty()) << "Compile error: " << err;

  LEPUSContext* ctx = qctx.context();
  LEPUSValue top_func = qctx.GetTopLevelFunction();

  // Get function list and find a function with var_defs
  uint32_t func_size = 0;
  LEPUSFunctionBytecode** func_list =
      GetDebuggerAllFunction(ctx, top_func, &func_size);
  ASSERT_NE(func_list, nullptr);

  LEPUSFunctionBytecode* target_b = nullptr;
  uint32_t target_idx = 0;
  for (uint32_t fi = 0; fi < func_size; ++fi) {
    if (func_list[fi] && GetFunctionVarDefCount(func_list[fi]) > 0) {
      target_b = func_list[fi];
      target_idx = fi;
      break;
    }
  }
  ASSERT_NE(target_b, nullptr) << "No function with var_defs found";

  // Step 1: Save original var_defs via C API
  uint32_t orig_count = GetFunctionVarDefCount(target_b);
  ASSERT_GT(orig_count, 0u);

  struct VarDefEntry {
    std::string name;
    int32_t scope_level;
    int32_t scope_next;
    uint8_t flags;
  };
  std::vector<VarDefEntry> originals(orig_count);
  for (uint32_t i = 0; i < orig_count; ++i) {
    const char* nm = GetFunctionVarDefName(ctx, target_b, i);
    originals[i].name = nm ? nm : "";
    if (nm && !LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, nm);
    originals[i].scope_level = GetFunctionVarDefScopeLevel(target_b, i);
    originals[i].scope_next = GetFunctionVarDefScopeNext(target_b, i);
    originals[i].flags = GetFunctionVarDefFlags(target_b, i);
  }

  // Step 2: Serialize to JSON
  std::string json_str = QuickjsDebugInfoBuilder::BuildJsDebugInfo(
      ctx, top_func, src, /*debuginfo_outside=*/true,
      /*var_defs_outside=*/true);

  // Step 3: Parse JSON and extract var_defs for target function
  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("function_info"));
  ASSERT_GT(doc["function_info"].Size(), target_idx);

  const auto& func_info = doc["function_info"][target_idx];
  ASSERT_TRUE(func_info.HasMember("vd"));
  const auto& var_defs_arr = func_info["vd"];
  ASSERT_EQ(var_defs_arr.Size(), orig_count);

  // Step 4: Parse JSON into arrays — mimics the deserialization in
  // FillFunctionBytecodeDebugInfo (lepusng_debugger.cc)
  std::vector<std::string> name_storage(orig_count);
  std::vector<const char*> var_names(orig_count);
  std::vector<int32_t> scope_levels(orig_count);
  std::vector<int32_t> scope_next_info(orig_count);
  std::vector<uint8_t> flags_vec(orig_count);

  for (uint32_t i = 0; i < orig_count; ++i) {
    const auto& vd = var_defs_arr[i];
    ASSERT_TRUE(vd.HasMember("n"));
    ASSERT_TRUE(vd.HasMember("sl"));
    ASSERT_TRUE(vd.HasMember("sn"));
    ASSERT_TRUE(vd.HasMember("f"));
    name_storage[i] = vd["n"].GetString();
    var_names[i] = name_storage[i].c_str();
    scope_levels[i] = vd["sl"].GetInt();
    scope_next_info[i] = vd["sn"].GetInt();
    flags_vec[i] = static_cast<uint8_t>(vd["f"].GetInt() & 0xFF);
  }

  // Step 5: Verify parsed JSON values match the originals
  for (uint32_t i = 0; i < orig_count; ++i) {
    EXPECT_EQ(name_storage[i], originals[i].name)
        << "Name mismatch at index " << i;
    EXPECT_EQ(scope_levels[i], originals[i].scope_level)
        << "Scope level mismatch at index " << i;
    EXPECT_EQ(scope_next_info[i], originals[i].scope_next)
        << "Scope next mismatch at index " << i;
    EXPECT_EQ(flags_vec[i], originals[i].flags)
        << "Flags mismatch at index " << i;
  }

  // Step 6: Call SetFunctionVarDefs to exercise the restore API path.
  // This validates the parsed format is accepted by the restore API.
  SetFunctionVarDefs(ctx, target_b, var_names.data(), scope_levels.data(),
                     scope_next_info.data(), flags_vec.data(), orig_count);

  // Step 7: After restore, verify getters still return consistent data.
  EXPECT_EQ(GetFunctionVarDefCount(target_b), orig_count);
  for (uint32_t i = 0; i < orig_count; ++i) {
    const char* nm = GetFunctionVarDefName(ctx, target_b, i);
    std::string restored_name = nm ? nm : "";
    if (nm && !LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, nm);
    EXPECT_EQ(restored_name, originals[i].name)
        << "Restored name mismatch at index " << i;
    EXPECT_EQ(GetFunctionVarDefScopeLevel(target_b, i),
              originals[i].scope_level)
        << "Restored scope_level mismatch at index " << i;
    EXPECT_EQ(GetFunctionVarDefScopeNext(target_b, i), originals[i].scope_next)
        << "Restored scope_next mismatch at index " << i;
    EXPECT_EQ(GetFunctionVarDefFlags(target_b, i), originals[i].flags)
        << "Restored flags mismatch at index " << i;
  }

  if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, func_list);
}

// Test: SetFunctionVarDefs on a second compilation restores equivalent
// var_defs. Simulates the debugger scenario: compile same source in a new
// context, then apply externally-stored var_defs.
TEST_F(QuickjsDebugInfoVarDefTest, RestoreVarDefsOnFreshCompilation) {
  // --- Phase 1: compile and extract var_defs to JSON ---
  std::string src =
      "function compute(x) {\n"
      "  let result = 0;\n"
      "  var temp = x * 2;\n"
      "  {\n"
      "    let inner = temp + 1;\n"
      "    result = inner;\n"
      "  }\n"
      "  return result;\n"
      "}\n";

  std::string json_str;
  {
    QuickContext qctx;
    qctx.set_debuginfo_outside(true);
    std::string err = BytecodeGenerator::GenerateBytecode(&qctx, src, "4.1");
    ASSERT_TRUE(err.empty()) << "Compile error: " << err;
    json_str = QuickjsDebugInfoBuilder::BuildJsDebugInfo(
        qctx.context(), qctx.GetTopLevelFunction(), src,
        /*debuginfo_outside=*/true, /*var_defs_outside=*/true);
  }

  // --- Phase 2: compile again in a fresh context ---
  QuickContext qctx2;
  qctx2.set_debuginfo_outside(true);
  std::string err2 = BytecodeGenerator::GenerateBytecode(&qctx2, src, "4.1");
  ASSERT_TRUE(err2.empty()) << "Compile error: " << err2;

  LEPUSContext* ctx2 = qctx2.context();
  LEPUSValue top_func2 = qctx2.GetTopLevelFunction();

  uint32_t func_size2 = 0;
  LEPUSFunctionBytecode** func_list2 =
      GetDebuggerAllFunction(ctx2, top_func2, &func_size2);
  ASSERT_NE(func_list2, nullptr);

  // --- Phase 3: parse the JSON and apply SetFunctionVarDefs ---
  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("function_info"));
  ASSERT_EQ(doc["function_info"].Size(), func_size2);

  for (uint32_t fi = 0; fi < func_size2; ++fi) {
    LEPUSFunctionBytecode* b = func_list2[fi];
    if (!b) continue;
    const auto& func_json = doc["function_info"][fi];
    if (!func_json.HasMember("vd") || !func_json["vd"].IsArray()) continue;

    const auto& var_defs_arr = func_json["vd"];
    uint32_t count = var_defs_arr.Size();
    if (count == 0) continue;

    std::vector<std::string> name_store(count);
    std::vector<const char*> names(count);
    std::vector<int32_t> levels(count);
    std::vector<int32_t> next_info(count);
    std::vector<uint8_t> flags(count);

    for (uint32_t i = 0; i < count; ++i) {
      const auto& vd = var_defs_arr[i];
      ASSERT_TRUE(vd.HasMember("n"));
      ASSERT_TRUE(vd.HasMember("sl"));
      ASSERT_TRUE(vd.HasMember("sn"));
      ASSERT_TRUE(vd.HasMember("f"));
      name_store[i] = vd["n"].GetString();
      names[i] = name_store[i].c_str();
      levels[i] = vd["sl"].GetInt();
      next_info[i] = vd["sn"].GetInt();
      flags[i] = static_cast<uint8_t>(vd["f"].GetInt() & 0xFF);
    }

    SetFunctionVarDefs(ctx2, b, names.data(), levels.data(), next_info.data(),
                       flags.data(), count);

    // Verify restoration via getters
    EXPECT_EQ(GetFunctionVarDefCount(b), count);
    for (uint32_t i = 0; i < count; ++i) {
      const char* nm = GetFunctionVarDefName(ctx2, b, i);
      std::string got_name = nm ? nm : "";
      if (nm && !LEPUS_IsGCMode(ctx2)) LEPUS_FreeCString(ctx2, nm);
      EXPECT_EQ(got_name, name_store[i])
          << "func " << fi << " var " << i << " name mismatch";
      EXPECT_EQ(GetFunctionVarDefScopeLevel(b, i), levels[i])
          << "func " << fi << " var " << i << " scope_level mismatch";
      EXPECT_EQ(GetFunctionVarDefScopeNext(b, i), next_info[i])
          << "func " << fi << " var " << i << " scope_next mismatch";
      EXPECT_EQ(GetFunctionVarDefFlags(b, i), flags[i])
          << "func " << fi << " var " << i << " flags mismatch";
    }
  }

  if (!LEPUS_IsGCMode(ctx2)) lepus_free(ctx2, func_list2);
}

// Test: function with no local variables does not produce "vd" key.
TEST_F(QuickjsDebugInfoVarDefTest, EmptyFunction_NoVarDefKey) {
  std::string src = "function empty() { return 42; }\n";

  std::string json_str = BuildDebugJson(src, /*var_defs_outside=*/true);

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("function_info"));

  // Find the "empty" function and verify no "vd"
  for (rapidjson::SizeType i = 0; i < doc["function_info"].Size(); ++i) {
    const auto& fi = doc["function_info"][i];
    if (fi.HasMember("function_name") &&
        std::string(fi["function_name"].GetString()) == "empty") {
      EXPECT_FALSE(fi.HasMember("vd"))
          << "Function 'empty' with no vars should not have 'vd'";
    }
  }
}

TEST_F(QuickjsDebugInfoVarDefTest, CompactSchemaUsesShortKeys) {
  QuickContext qctx;
  qctx.set_debuginfo_outside(true);
  std::string src =
      "function foo(a) {\n"
      "  let local = a + 1;\n"
      "  return local;\n"
      "}\n";
  std::string err = BytecodeGenerator::GenerateBytecode(&qctx, src, "4.2");
  ASSERT_TRUE(err.empty()) << "Compile error: " << err;

  std::string json_str = QuickjsDebugInfoBuilder::BuildJsDebugInfo(
      qctx.context(), qctx.GetTopLevelFunction(), src,
      /*debuginfo_outside=*/true, /*var_defs_outside=*/true,
      /*compact_debug_info=*/true);

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("v"));
  EXPECT_EQ(doc["v"].GetInt(), 2);
  EXPECT_TRUE(doc.HasMember("fs"));
  EXPECT_TRUE(doc.HasMember("el"));
  EXPECT_TRUE(doc.HasMember("fn"));
  ASSERT_TRUE(doc.HasMember("fi"));
  ASSERT_TRUE(doc["fi"].IsArray());
  EXPECT_FALSE(doc.HasMember("function_source"));
  EXPECT_FALSE(doc.HasMember("function_number"));
  EXPECT_FALSE(doc.HasMember("function_info"));

  ASSERT_GT(doc["fi"].Size(), 1u);
  const auto& child = doc["fi"][1];
  EXPECT_TRUE(child.HasMember("id"));
  EXPECT_TRUE(child.HasMember("n"));
  EXPECT_TRUE(child.HasMember("ln"));
  EXPECT_TRUE(child.HasMember("cn"));
  EXPECT_TRUE(child.HasMember("lc"));
  EXPECT_TRUE(child.HasMember("pll"));
  EXPECT_TRUE(child.HasMember("plb"));
  EXPECT_TRUE(child.HasMember("pci"));
  EXPECT_TRUE(child.HasMember("fsl"));
  EXPECT_TRUE(child.HasMember("fso"));
  EXPECT_FALSE(child.HasMember("function_id"));
  EXPECT_FALSE(child.HasMember("function_name"));
  EXPECT_FALSE(child.HasMember("line_number"));
  EXPECT_FALSE(child.HasMember("column_number"));
  EXPECT_FALSE(child.HasMember("function_source"));
  EXPECT_FALSE(child.HasMember("function_source_offset"));
  EXPECT_FALSE(child.HasMember("fs"));
  ASSERT_TRUE(child["fso"].IsInt());
  ASSERT_TRUE(child["fsl"].IsInt());
  int32_t source_offset = child["fso"].GetInt();
  int32_t source_len = child["fsl"].GetInt();
  ASSERT_GE(source_offset, 0);
  ASSERT_GE(source_len, 0);
  size_t source_offset_size = static_cast<size_t>(source_offset);
  size_t source_len_size = static_cast<size_t>(source_len);
  ASSERT_LE(source_offset_size + source_len_size, src.size());
  EXPECT_EQ(src.substr(source_offset, source_len),
            "function foo(a) {\n"
            "  let local = a + 1;\n"
            "  return local;\n"
            "}");

  if (child["lc"].IsArray() && child["lc"].Size() > 0) {
    EXPECT_TRUE(child["lc"][0].HasMember("l"));
    EXPECT_TRUE(child["lc"][0].HasMember("c"));
    EXPECT_FALSE(child["lc"][0].HasMember("line"));
    EXPECT_FALSE(child["lc"][0].HasMember("column"));
  }
}

TEST_F(QuickjsDebugInfoVarDefTest, CompactSchemaSourceOffsetsMatchRootSource) {
  QuickContext qctx;
  qctx.set_debuginfo_outside(true);
  std::string src =
      "const prefix = 1;\n"
      "class Base {}\n"
      "class Derived extends Base {}\n"
      "function outer(a) {\n"
      "  const arrow = (b) => b + a;\n"
      "  class Box {\n"
      "    constructor(value) {\n"
      "      this.value = value;\n"
      "    }\n"
      "    method(delta) {\n"
      "      return arrow(this.value + delta);\n"
      "    }\n"
      "  }\n"
      "  return new Box(prefix).method(2);\n"
      "}\n"
      "outer(3);\n";
  std::string err = BytecodeGenerator::GenerateBytecode(&qctx, src, "4.2");
  ASSERT_TRUE(err.empty()) << "Compile error: " << err;

  std::string json_str = QuickjsDebugInfoBuilder::BuildJsDebugInfo(
      qctx.context(), qctx.GetTopLevelFunction(), src,
      /*debuginfo_outside=*/true, /*var_defs_outside=*/true,
      /*compact_debug_info=*/true);

  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.HasMember("fi"));
  ASSERT_TRUE(doc["fi"].IsArray());

  auto expect_source = [&](const std::string& expected_source) {
    SCOPED_TRACE(expected_source);
    auto expected_offset = src.find(expected_source);
    ASSERT_NE(expected_offset, std::string::npos);
    bool found = false;
    for (const auto& function_info : doc["fi"].GetArray()) {
      if (!function_info.HasMember("fso") || !function_info.HasMember("fsl")) {
        continue;
      }
      int32_t source_offset = function_info["fso"].GetInt();
      int32_t source_len = function_info["fsl"].GetInt();
      ASSERT_GE(source_offset, 0);
      ASSERT_GE(source_len, 0);
      ASSERT_LE(
          static_cast<size_t>(source_offset) + static_cast<size_t>(source_len),
          src.size());
      if (source_offset == static_cast<int32_t>(expected_offset) &&
          source_len == static_cast<int32_t>(expected_source.length())) {
        EXPECT_EQ(src.substr(source_offset, source_len), expected_source);
        EXPECT_FALSE(function_info.HasMember("fs"));
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found);
  };

  expect_source("class Base {}");
  expect_source("class Derived extends Base {}");
  expect_source(
      "function outer(a) {\n"
      "  const arrow = (b) => b + a;\n"
      "  class Box {\n"
      "    constructor(value) {\n"
      "      this.value = value;\n"
      "    }\n"
      "    method(delta) {\n"
      "      return arrow(this.value + delta);\n"
      "    }\n"
      "  }\n"
      "  return new Box(prefix).method(2);\n"
      "}");
  expect_source("(b) => b + a");
  expect_source(
      "class Box {\n"
      "    constructor(value) {\n"
      "      this.value = value;\n"
      "    }\n"
      "    method(delta) {\n"
      "      return arrow(this.value + delta);\n"
      "    }\n"
      "  }");
  expect_source(
      "method(delta) {\n"
      "      return arrow(this.value + delta);\n"
      "    }");
}

TEST_F(QuickjsDebugInfoVarDefTest, AddDebugInfoUsesCompactSchemaFromSdk42) {
  std::string src =
      "function foo() {\n"
      "  return 1;\n"
      "}\n";

  rapidjson::Document sdk41_doc = BuildDebugInfoThroughBuilder(src, "4.1");
  ASSERT_TRUE(sdk41_doc.HasMember("lepus.js"));
  const auto& sdk41_entry = sdk41_doc["lepus.js"];
  EXPECT_FALSE(sdk41_entry.HasMember("v"));
  EXPECT_TRUE(sdk41_entry.HasMember("function_source"));
  EXPECT_TRUE(sdk41_entry.HasMember("function_number"));
  EXPECT_TRUE(sdk41_entry.HasMember("function_info"));
  EXPECT_FALSE(sdk41_entry.HasMember("fs"));
  EXPECT_FALSE(sdk41_entry.HasMember("fn"));
  EXPECT_FALSE(sdk41_entry.HasMember("fi"));

  rapidjson::Document sdk42_doc = BuildDebugInfoThroughBuilder(src, "4.2");
  ASSERT_TRUE(sdk42_doc.HasMember("lepus.js"));
  const auto& sdk42_entry = sdk42_doc["lepus.js"];
  ASSERT_TRUE(sdk42_entry.HasMember("v"));
  EXPECT_EQ(sdk42_entry["v"].GetInt(), 2);
  EXPECT_TRUE(sdk42_entry.HasMember("fs"));
  EXPECT_TRUE(sdk42_entry.HasMember("fn"));
  EXPECT_TRUE(sdk42_entry.HasMember("fi"));
  EXPECT_FALSE(sdk42_entry.HasMember("function_source"));
  EXPECT_FALSE(sdk42_entry.HasMember("function_number"));
  EXPECT_FALSE(sdk42_entry.HasMember("function_info"));
}

}  // namespace lepus
}  // namespace lynx
