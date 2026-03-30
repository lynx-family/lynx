// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <sys/types.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#define private public

#include "base/include/value/base_value.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/context_binary_writer.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
#include "testing/lynx/tasm/databinding/databinding_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

const static std::string target_sdk_version = "2.6";
namespace lynx {
namespace tasm {

class TestUtils {
 public:
  static std::vector<std::string> GetTestFileLists(const std::string& folder) {
    std::vector<std::string> all_file;

    std::filesystem::path folder_path(folder);
    if (!std::filesystem::exists(folder_path) ||
        !std::filesystem::is_directory(folder_path)) {
      return all_file;
    }

    for (const auto& entry : std::filesystem::directory_iterator(
             folder_path,
             std::filesystem::directory_options::skip_permission_denied)) {
      all_file.push_back(entry.path());
    }

    return all_file;
  }
  static std::string ReadFileFromPath(const std::string& filename) {
    std::ifstream instream(filename, std::ios::in);
    if (!instream.is_open()) {
      std::cout << "file open failed" << std::endl;
      return {};
    }
    return std::string((std::istreambuf_iterator<char>(instream)),
                       std::istreambuf_iterator<char>());
  }

  static void RegisterBuiltin(runtime::MTSRuntime* ctx) {
    if (ctx->IsVMContext()) {
      lepus::VMContext* vm_ctx = runtime::MTSRuntime::ToVMContext(ctx);
      vm_ctx->Initialize();
      vm_ctx->SetClosureFix(true);
      lepus::RegisterCFunction(vm_ctx, "Assert", Assert);
      lepus::RegisterCFunction(vm_ctx, "print", Print);
      lepus::RegisterCFunction(vm_ctx, "typeof", Typeof);
    } else {
      lepus::RegisterNGCFunction(runtime::MTSRuntime::ToQuickContext(ctx),
                                 "Assert", Assert);
    }
  }

  static bool HasLepusNullPropAsUndefMarker(const std::string& source) {
    std::string kLepusNullPropAsUndefMarker = "//lepusNullPropAsUndef";
    size_t len = kLepusNullPropAsUndefMarker.size();
    if (source.size() < len) return false;
    return source.compare(0, len, kLepusNullPropAsUndefMarker) == 0;
  }

 private:
  static lepus::Value Assert(runtime::MTSContext* context, lepus::Value* val,
                             int argc) {
    if (val->IsTrue()) {
      return lepus::Value();
    } else {
      std::cout << "Test Failed!" << std::endl;
      abort();
    }
  }
  static lepus::Value Print(runtime::MTSContext* context, lepus::Value* val,
                            int params_count) {
    for (long i = 0; i < params_count; i++) {
      lepus::Value v(*static_cast<lepus::VMContext*>(context)->GetParam(i));
      v.Print();
    }
    return lepus::Value();
  }

  static lepus::Value Typeof(runtime::MTSContext* context, lepus::Value* val,
                             int argc) {
    switch (val->Type()) {
      case lepus::ValueType::Value_Nil:
        val->SetString("null");
        break;
      case lepus::ValueType::Value_Undefined:
        val->SetString("undefined");
        break;
      case lepus::ValueType::Value_Double:
      case lepus::ValueType::Value_Bool:
      case lepus::ValueType::Value_Int32:
      case lepus::ValueType::Value_Int64:
      case lepus::ValueType::Value_UInt32:
      case lepus::ValueType::Value_UInt64:
        val->SetString("number");
        break;
      case lepus::ValueType::Value_String:
        val->SetString("string");
        break;
      case lepus::ValueType::Value_Table:
        val->SetString("table");
        break;
      case lepus::ValueType::Value_Array:
        val->SetString("object");
        break;
      case lepus::ValueType::Value_Closure:
      case lepus::ValueType::Value_CFunction:
        val->SetString("function");
      case lepus::ValueType::Value_CPointer:
      case lepus::ValueType::Value_RefCounted:
        val->SetString("pointer");
        break;
      default:
        break;
    }
    return *val;
  }

  static LEPUSValue Assert(LEPUSContext* ctx, LEPUSValue this_obj, int32_t argc,
                           LEPUSValue* argv) {
    for (int32_t i = 0; i < argc; ++i) {
      auto value = LEPUS_ToBool(ctx, argv[i]);
      assert(value);

      if (!value) {
        abort();
      }
    }
    return LEPUS_UNDEFINED;
  }
};

class ContextBinaryWriterTest : public lepus::ContextBinaryWriter {
 public:
  explicit ContextBinaryWriterTest(runtime::MTSRuntime* ctx)
      : lepus::ContextBinaryWriter(
            ctx->GetMTSContext(),
            CompileOptions{.target_sdk_version_ = target_sdk_version}) {}

  void encode() {
    lepus::ContextBinaryWriter::encode();
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

class ContextBinaryReaderTest : public ::testing::Test {
 public:
  ContextBinaryReaderTest() = default;
};

class LynxBinaryReaderTest : public LynxBinaryReader {
 public:
  explicit LynxBinaryReaderTest(std::unique_ptr<lepus::InputStream> stream,
                                bool is_lepusng_binary)
      : LynxBinaryReader(std::move(stream)) {
    compile_options_.target_sdk_version_ = target_sdk_version;
    is_lepusng_binary_ = is_lepusng_binary;
  }

  ~LynxBinaryReaderTest() override = default;

  bool DecodeContextTest() {
    if (!is_lepusng_binary_) {
      uint8_t has_string_table = false;
      ReadU8(&has_string_table);
      if (has_string_table && !DeserializeStringSection()) {
        return false;
      }
    }
    return LynxBinaryReader::DecodeContext();
  }
};

class TemplateBinaryReaderTest : public test::DataBindingShell,
                                 public TemplateEntry,
                                 public TemplateBinaryReader {
 public:
  TemplateBinaryReaderTest(std::unique_ptr<lepus::InputStream> stream,
                           bool is_lepusng_binary)
      : TemplateEntry{runtime::MTSRuntime::CreateContext(
                          is_lepusng_binary
                              ? runtime::ContextType::LepusNGContextType
                              : runtime::ContextType::VMContextType),
                      target_sdk_version},
        TemplateBinaryReader(test::DataBindingShell::tasm_,
                             static_cast<TemplateEntry*>(this),
                             std::move(stream)) {
    TemplateBinaryReader::compile_options_.target_sdk_version_ =
        target_sdk_version;
    TestUtils::RegisterBuiltin(vm_context_.get());
  }

  bool DecodeContext() override {
    if (vm_context_->IsVMContext()) {
      uint8_t has_string_table = false;
      ReadU8(&has_string_table);
      if (has_string_table && !DeserializeStringSection()) {
        return false;
      }
    }
    return TemplateBinaryReader::DecodeContext();
  }
};

TEST_F(ContextBinaryReaderTest, LynxBinaryReaderLepus) {
  auto all_test_file =
      TestUtils::GetTestFileLists("core/runtime/lepus/compiler/unit_test/");

  if (all_test_file.empty()) {
    GTEST_SKIP()
        << "No test files found under core/runtime/lepus/compiler/unit_test/";
  }

  // Note: `core/runtime/lepus/compiler/unit_test/` contains a large number of
  // Lepus IR optimization-related test cases. Running all of them here will
  // easily exceed the unit test time budget (timeout).
  //
  // Those IR optimization cases are fully covered in `lepus_compiler_exec`
  // (which runs the entire set), so `LynxBinaryReaderLepus` only exercises the
  // legacy/non-IR-prefixed cases for binary reader regression coverage.
  constexpr const char* kLepusIrTestPrefix = "test_lepus_ir_";

  for (const auto& test_file : all_test_file) {
    const std::filesystem::path test_path(test_file);
    if (!std::filesystem::is_regular_file(test_path)) {
      continue;
    }
    const auto filename = test_path.filename().string();
    if (filename.rfind(kLepusIrTestPrefix, 0) == 0) {
      continue;
    }
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx =
        runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
    TestUtils::RegisterBuiltin(vm_ctx.get());
    runtime::MTSRuntime::ToVMContext(vm_ctx.get())
        ->SetNullPropAsUndef(TestUtils::HasLepusNullPropAsUndefMarker(src));
    lepus::BytecodeGenerator::GenerateBytecode(vm_ctx->GetMTSContext(), src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(vm_ctx.get());
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    auto binary_reader = LynxBinaryReaderTest(
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        false);
    ASSERT_TRUE(binary_reader.DecodeContextTest());
    std::shared_ptr<runtime::MTSRuntime> decode_ctx =
        runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
    auto entry = TemplateEntry(decode_ctx, target_sdk_version);
    TestUtils::RegisterBuiltin(decode_ctx.get());
    ASSERT_TRUE(decode_ctx->DeSerialize(
        *binary_reader.GetTemplateBundle().context_bundle_, true, nullptr));
    ASSERT_TRUE(decode_ctx->Execute(
        binary_reader.GetTemplateBundle().context_bundle_.get()));
  }
}

TEST_F(ContextBinaryReaderTest, LynxBinaryReaderLepusNG) {
  auto all_test_file = TestUtils::GetTestFileLists(
      "core/runtime/lepus/compiler/lepusng_unit_test");

  if (all_test_file.empty()) {
    GTEST_SKIP() << "No test files found under "
                    "core/runtime/lepus/compiler/lepusng_unit_test";
  }

  for (const auto& test_file : all_test_file) {
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx = runtime::MTSRuntime::CreateContext(
        runtime::ContextType::LepusNGContextType);
    lepus::BytecodeGenerator::GenerateBytecode(vm_ctx->GetMTSContext(), src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(vm_ctx.get());
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    auto binary_reader = LynxBinaryReaderTest(
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        true);
    ASSERT_TRUE(binary_reader.DecodeContextTest());
    std::shared_ptr<runtime::MTSRuntime> decode_ctx =
        runtime::MTSRuntime::CreateContext(
            runtime::ContextType::LepusNGContextType);
    TestUtils::RegisterBuiltin(decode_ctx.get());
    auto entry = TemplateEntry(decode_ctx, target_sdk_version);
    ASSERT_TRUE(entry.GetVm()->DeSerialize(
        *binary_reader.GetTemplateBundle().context_bundle_, false, nullptr));
    ASSERT_TRUE(decode_ctx->Execute(
        binary_reader.GetTemplateBundle().context_bundle_.get()));
  }
}

// TemplateBinaryReader has the same context construction method as
// LynxBinaryReader, so disable the following two tests
TEST_F(ContextBinaryReaderTest, DISABLED_TemplateBinaryReaderLepus) {
  auto all_test_file =
      TestUtils::GetTestFileLists("core/runtime/lepus/compiler/unit_test");

  for (const auto& test_file : all_test_file) {
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx =
        runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
    static_cast<lepus::VMContext*>(vm_ctx->GetMTSContext())
        ->SetOptBytecode(false);
    TestUtils::RegisterBuiltin(vm_ctx.get());
    lepus::BytecodeGenerator::GenerateBytecode(vm_ctx->GetMTSContext(), src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(vm_ctx.get());
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    TemplateBinaryReaderTest binary_reader{
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        false};

    ASSERT_TRUE(binary_reader.DecodeContext());
    ASSERT_TRUE(binary_reader.GetVm()->Execute(
        binary_reader.GetTemplateBundle().context_bundle_.get()));
  }
}

TEST_F(ContextBinaryReaderTest, DISABLED_TemplateBinaryReaderLepusOpt) {
  auto all_test_file =
      TestUtils::GetTestFileLists("core/runtime/lepus/compiler/unit_test");

  for (const auto& test_file : all_test_file) {
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx =
        runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
    static_cast<lepus::VMContext*>(vm_ctx->GetMTSContext())
        ->SetOptBytecode(true);
    TestUtils::RegisterBuiltin(vm_ctx.get());
    lepus::BytecodeGenerator::GenerateBytecode(vm_ctx->GetMTSContext(), src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(vm_ctx.get());
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    TemplateBinaryReaderTest binary_reader{
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        false};

    ASSERT_TRUE(binary_reader.DecodeContext());
    ASSERT_TRUE(binary_reader.GetVm()->Execute(
        binary_reader.GetTemplateBundle().context_bundle_.get()));
  }
}

TEST_F(ContextBinaryReaderTest, DISABLED_TemplateBinaryReaderLepusNG) {
  auto all_test_file = TestUtils::GetTestFileLists(
      "core/runtime/lepus/compiler/lepusng_unit_test/");

  for (const auto& test_file : all_test_file) {
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx = runtime::MTSRuntime::CreateContext(
        runtime::ContextType::LepusNGContextType);
    TestUtils::RegisterBuiltin(vm_ctx.get());
    lepus::BytecodeGenerator::GenerateBytecode(vm_ctx->GetMTSContext(), src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(vm_ctx.get());
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    TemplateBinaryReaderTest binary_reader{
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        true};

    ASSERT_TRUE(binary_reader.DecodeContext());
    ASSERT_TRUE(binary_reader.GetVm()->Execute(
        binary_reader.GetTemplateBundle().context_bundle_.get()));
  }
}

}  // namespace tasm
}  // namespace lynx
