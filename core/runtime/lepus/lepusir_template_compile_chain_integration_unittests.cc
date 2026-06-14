// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/context_binary_writer.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
#include "testing/lynx/tasm/databinding/databinding_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

namespace {

static bool ShouldEnableOptBytecode(bool encode_opt_lepus_code,
                                    const std::string& target_sdk_version) {
  if (!encode_opt_lepus_code) {
    return false;
  }
  return lynx::tasm::Config::IsHigherOrEqual(target_sdk_version,
                                             FEATURE_OPT_LEPUS_BYTECODE);
}

static void PrepareVMContext(runtime::MTSRuntime* ctx) {
  if (!ctx || !ctx->IsVMContext()) {
    return;
  }
  // Ensure underlying VMContext is initialized and ready for execution.
  ctx->Initialize();
  if (auto* vm = runtime::MTSRuntime::ToVMContext(ctx)) {
    vm->SetClosureFix(true);
  }
}

class ContextBinaryWriterWithStringTableForTest
    : public lepus::ContextBinaryWriter {
 public:
  explicit ContextBinaryWriterWithStringTableForTest(
      runtime::MTSRuntime* ctx, const std::string& target_sdk_version)
      : lepus::ContextBinaryWriter(
            ctx->GetMTSContext(),
            CompileOptions{.target_sdk_version_ = target_sdk_version}) {}

  void Encode() {
    lepus::ContextBinaryWriter::encode();

    // Keep compatible with existing VMContext binary decode path that may
    // optionally deserialize a string table section before the context bundle.
    if (IsVMContext()) {
      auto* string_table = mts_context()->string_table();
      size_t str_sec_offset = Offset();
      WriteByte(string_table->string_list.size() ? true : false);

      if (string_table->string_list.size()) {
        WriteCompactU32(string_table->string_list.size());
        for (const auto& i : string_table->string_list) {
          auto str = i.str();
          size_t length = str.length();
          WriteCompactU32(length);
          if (length) {
            stream_->WriteData(reinterpret_cast<const uint8_t*>(str.c_str()),
                               length);
          }
        }
      }
      Move(0, str_sec_offset, Offset() - str_sec_offset);
    }
  }
};

class BaseBinaryReaderForContextBundleTest : public lepus::BaseBinaryReader {
 public:
  explicit BaseBinaryReaderForContextBundleTest(
      std::unique_ptr<lepus::InputStream> stream,
      const std::string& target_sdk_version)
      : lepus::BaseBinaryReader(std::move(stream)) {
    compile_options_.target_sdk_version_ = target_sdk_version;
  }
};

static bool RunEncodeDecodeVmExecuteAndGetGlobal(
    const std::string& src, bool encode_opt_lepus_code,
    const std::string& target_sdk_version, const char* global_name,
    lepus::Value* out, bool expect_opt_lepus_code) {
  auto encode_ctx =
      runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
  PrepareVMContext(encode_ctx.get());
  encode_ctx->SetSdkVersion(target_sdk_version);
  bool opt_bytecode_enabled =
      ShouldEnableOptBytecode(encode_opt_lepus_code, target_sdk_version);
  if (auto* vm = runtime::MTSRuntime::ToVMContext(encode_ctx.get())) {
    vm->SetOptBytecode(opt_bytecode_enabled);
  }

  EXPECT_EQ(opt_bytecode_enabled, expect_opt_lepus_code);

  lepus::BytecodeGenerator::GenerateBytecode(encode_ctx->GetMTSContext(), src,
                                             target_sdk_version, "");

  auto writer = ContextBinaryWriterWithStringTableForTest(encode_ctx.get(),
                                                          target_sdk_version);
  writer.Encode();
  std::vector<uint8_t> byte_array = writer.byte_array();

  auto reader = BaseBinaryReaderForContextBundleTest(
      std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
      target_sdk_version);
  uint8_t has_string_table = false;
  if (!reader.ReadU8(&has_string_table)) {
    return false;
  }
  if (has_string_table && !reader.DeserializeStringSection()) {
    return false;
  }
  auto bundle =
      runtime::ContextBundle::Create(runtime::ContextType::VMContextType);
  if (!bundle) {
    return false;
  }
  if (!reader.DecodeContextBundle(bundle.get())) {
    return false;
  }

  auto decode_ctx =
      runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
  PrepareVMContext(decode_ctx.get());
  decode_ctx->SetSdkVersion(target_sdk_version);

  if (!decode_ctx->DeSerialize(*bundle, true, nullptr)) {
    return false;
  }
  if (!decode_ctx->Execute(bundle.get())) {
    return false;
  }

  // `var result = ...` is a top-level variable in VMContext, not a global data.
  if (!decode_ctx->GetTopLevelVariableByName(lynx::base::String(global_name),
                                             out)) {
    return false;
  }
  return true;
}

}  // namespace

TEST(LEPUS_IR_TEST_OPT_BYTECODE_SWITCH,
     TemplateCompileChain_OptOnOff_SemanticEq_RepresentativeCases) {
  struct Case {
    const char* name;
    const char* src;
    lepus::Value expected;
  };

  // The script assigns the final answer to global `result`.
  static const Case kCases[] = {
      {
          "arith_branch",
          R"(
var a = 1;
var b = 2;
var c = a + b * 3;
if (c > 5) { c = c - 1; } else { c = c + 1; }
var result = c;
)",
          lepus::Value(6),
      },
      {
          "closure_capture_mutation",
          R"(
var x = 1;
function make() {
  var y = 2;
  return function(z) {
    x = x + 1;
    return x + y + z;
  };
}
var f = make();
var result = f(3) + f(4);
)",
          lepus::Value(16),
      },
      {
          "loop_array_sum",
          R"(
var arr = [1, 2, 3, 4];
var sum = 0;
for (var i = 0; i < arr.length; i = i + 1) {
  sum = sum + arr[i];
}
var result = sum;
)",
          lepus::Value(10),
      },
      {
          "short_circuit_side_effects",
          R"(
var x = 0;
function inc() { x = x + 1; return x; }
var t1 = (0 && inc());
var t2 = (1 || inc());
var result = x;
)",
          lepus::Value(0),
      },
      {
          "toplevel_function_expr_call",
          R"(
var f = function(a) { return a + 10; };
var g = function() { return f(1) + f(2); };
var result = g();
)",
          lepus::Value(23),
      },
  };

  for (const auto& c : kCases) {
    // Match encoder gating semantics:
    //   SetOptBytecode(true) iff encodeOptLepusCode == true && targetSdk >= 3.8
    lepus::Value res_sdk26_opt_flag_false;
    lepus::Value res_sdk26_opt_flag_true;
    ASSERT_TRUE(RunEncodeDecodeVmExecuteAndGetGlobal(
        c.src, false, "2.6", "result", &res_sdk26_opt_flag_false, false));
    ASSERT_TRUE(RunEncodeDecodeVmExecuteAndGetGlobal(
        c.src, true, "2.6", "result", &res_sdk26_opt_flag_true, false));
    EXPECT_TRUE(res_sdk26_opt_flag_false == c.expected);
    EXPECT_TRUE(res_sdk26_opt_flag_true == c.expected);
    EXPECT_TRUE(res_sdk26_opt_flag_false == res_sdk26_opt_flag_true);

    lepus::Value res_sdk38_opt_flag_false;
    lepus::Value res_sdk38_opt_flag_true;
    ASSERT_TRUE(RunEncodeDecodeVmExecuteAndGetGlobal(
        c.src, false, "3.8", "result", &res_sdk38_opt_flag_false, false));
    ASSERT_TRUE(RunEncodeDecodeVmExecuteAndGetGlobal(
        c.src, true, "3.8", "result", &res_sdk38_opt_flag_true, true));
    EXPECT_TRUE(res_sdk38_opt_flag_false == c.expected);
    EXPECT_TRUE(res_sdk38_opt_flag_true == c.expected);
    EXPECT_TRUE(res_sdk38_opt_flag_false == res_sdk38_opt_flag_true);
  }
}

TEST(LEPUS_IR_TEST_OPT_BYTECODE_SWITCH,
     TemplateCompileChain_DoubleNotOrAndChain_SemanticEq) {
  struct Case {
    const char* name;
    const char* src;
    lepus::Value expected;
  };

  // Exercise !!x || !!y and !!x && !!y patterns that the IsInBooleanContext
  // optimization now folds.
  static const Case kCases[] = {
      {
          "double_not_or_both_truthy",
          R"(
var a = 1;
var b = 2;
var result = (!!a || !!b) ? 10 : 20;
)",
          lepus::Value(10),
      },
      {
          "double_not_or_first_falsy",
          R"(
var a = 0;
var b = 5;
var result = (!!a || !!b) ? 10 : 20;
)",
          lepus::Value(10),
      },
      {
          "double_not_or_both_falsy",
          R"(
var a = 0;
var b = 0;
var result = (!!a || !!b) ? 10 : 20;
)",
          lepus::Value(20),
      },
      {
          "double_not_and_both_truthy",
          R"(
var a = 1;
var b = 2;
var result = (!!a && !!b) ? 10 : 20;
)",
          lepus::Value(10),
      },
      {
          "double_not_and_first_falsy",
          R"(
var a = 0;
var b = 5;
var result = (!!a && !!b) ? 10 : 20;
)",
          lepus::Value(20),
      },
      {
          "double_not_nested_or_and",
          R"(
var a = 1;
var b = 0;
var c = 3;
var result = (!!a || !!b) && !!c ? 10 : 20;
)",
          lepus::Value(10),
      },
  };

  for (const auto& c : kCases) {
    // opt-off (sdk 2.6)
    lepus::Value res_off;
    ASSERT_TRUE(RunEncodeDecodeVmExecuteAndGetGlobal(c.src, false, "2.6",
                                                     "result", &res_off, false))
        << "case: " << c.name;
    EXPECT_TRUE(res_off == c.expected) << "case (off): " << c.name;

    // opt-on (sdk 3.8)
    lepus::Value res_on;
    ASSERT_TRUE(RunEncodeDecodeVmExecuteAndGetGlobal(c.src, true, "3.8",
                                                     "result", &res_on, true))
        << "case: " << c.name;
    EXPECT_TRUE(res_on == c.expected) << "case (on): " << c.name;

    // opt-on and opt-off produce same results
    EXPECT_TRUE(res_off == res_on) << "case (eq): " << c.name;
  }
}

// Mock _GetLength: returns array size as int (int32_t), matching the real
// renderer_functions.cc implementation.
static lepus::Value GetLengthMock(lepus::MTSContext*, lepus::Value* args,
                                  int argc) {
  if (argc < 1 || !args) return lepus::Value(0);
  if (args[0].IsArray()) {
    int len = static_cast<int>(args[0].Array()->size());
    return lepus::Value(len);  // Value(int32_t) → lynx_value_int32
  }
  return lepus::Value(0);
}

static void RegisterGetLengthForTest(runtime::MTSRuntime* ctx) {
  if (!ctx || !ctx->IsVMContext()) return;
  if (auto* vm = runtime::MTSRuntime::ToVMContext(ctx)) {
    lepus::RegisterCFunction(vm, "_GetLength", &GetLengthMock);
  }
}

// Regression: _GetLength returns Value_Int32 at runtime. If the IR optimizer
// incorrectly annotates it as Int64, instruction selection emits
// TypeOp_GetArrayInt64 which reads val_int64 from a Value_Int32, producing
// garbage or crashing.
TEST(LEPUS_IR_TEST_OPT_BYTECODE_SWITCH, GetLengthResultAsArrayIndex_NoCrash) {
  const char* src = R"(
var arr = [10, 20, 30];
var len = _GetLength(arr);
var result = arr[len - 1];
)";

  // Encode with opt enabled
  auto encode_ctx =
      runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
  PrepareVMContext(encode_ctx.get());
  encode_ctx->SetSdkVersion("3.8");
  RegisterGetLengthForTest(encode_ctx.get());
  if (auto* vm = runtime::MTSRuntime::ToVMContext(encode_ctx.get())) {
    vm->SetOptBytecode(true);
  }

  lepus::BytecodeGenerator::GenerateBytecode(encode_ctx->GetMTSContext(), src,
                                             "3.8", "");

  auto writer =
      ContextBinaryWriterWithStringTableForTest(encode_ctx.get(), "3.8");
  writer.Encode();
  std::vector<uint8_t> byte_array = writer.byte_array();

  // Decode and execute
  auto reader = BaseBinaryReaderForContextBundleTest(
      std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
      "3.8");
  uint8_t has_string_table = false;
  ASSERT_TRUE(reader.ReadU8(&has_string_table));
  if (has_string_table) {
    ASSERT_TRUE(reader.DeserializeStringSection());
  }
  auto bundle =
      runtime::ContextBundle::Create(runtime::ContextType::VMContextType);
  ASSERT_NE(nullptr, bundle);
  ASSERT_TRUE(reader.DecodeContextBundle(bundle.get()));

  auto decode_ctx =
      runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
  PrepareVMContext(decode_ctx.get());
  decode_ctx->SetSdkVersion("3.8");
  RegisterGetLengthForTest(decode_ctx.get());

  ASSERT_TRUE(decode_ctx->DeSerialize(*bundle, true, nullptr));
  ASSERT_TRUE(decode_ctx->Execute(bundle.get()));

  lepus::Value out;
  ASSERT_TRUE(decode_ctx->GetTopLevelVariableByName(
      lynx::base::String("result"), &out));
  // arr[3-1] == arr[2] == 30
  EXPECT_TRUE(out == lepus::Value(30))
      << "Expected 30 when using _GetLength result as array index";
}

}  // namespace tasm
}  // namespace lynx
