// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <sys/types.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#define private public

#include "core/renderer/page_config.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/context_binary_writer.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
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

    for (const auto& entry :
         std::filesystem::directory_iterator(std::filesystem::path(folder))) {
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

  static void RegisterBuiltin(lepus::Context* ctx) {
    if (ctx->IsVMContext()) {
      ctx->Initialize();
      static_cast<lepus::VMContext*>(ctx)->SetClosureFix(true);
      lepus::RegisterCFunction(ctx, "Assert", Assert);
      lepus::RegisterCFunction(ctx, "print", Print);
      lepus::RegisterCFunction(ctx, "setFlag", SetFlag);
      lepus::RegisterCFunction(ctx, "typeof", Typeof);
    } else {
      lepus::RegisterNGCFunction(ctx, "Assert", Assert);
    }
  }

 private:
  static lepus::Value Assert(lepus::Context* context) {
    lepus::Value* val = context->GetParam(0);
    if (val->IsTrue()) {
      return lepus::Value();
    } else {
      std::cout << "Test Failed!" << std::endl;
      abort();
    }
  }
  static lepus::Value Print(lepus::Context* context) {
    long params_count = context->GetParamsSize();
    for (long i = 0; i < params_count; i++) {
      lepus::Value* v = context->GetParam(i);
      v->Print();
    }
    return lepus::Value();
  }

  static lepus::Value Typeof(lepus::Context* context) {
    lepus::Value* val = context->GetParam(0);
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

  static lepus::Value SetFlag(lepus::Context* context) {
    lepus::Value* parm1 = context->GetParam(0);
    if (parm1->String().IsEqual("lepusNullPropAsUndef")) {
      lepus::VMContext::Cast(context)->SetNullPropAsUndef(
          context->GetParam(1)->Bool());
    }
    return lepus::Value();
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
  explicit ContextBinaryWriterTest(lepus::Context* ctx)
      : lepus::ContextBinaryWriter(
            ctx, CompileOptions{.target_sdk_version_ = target_sdk_version}) {}

  void encode() {
    lepus::ContextBinaryWriter::encode();
    if (context_->IsVMContext()) {
      size_t str_sec_offset = Offset();
      WriteByte(context_->string_table()->string_list.size() ? true : false);

      if (context_->string_table()->string_list.size()) {
        WriteCompactU32(context_->string_table()->string_list.size());

        for (const auto& i : context_->string_table()->string_list) {
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
      : TemplateEntry{lepus::Context::CreateContext(is_lepusng_binary),
                      target_sdk_version},
        TemplateBinaryReader(test::DataBindingShell::tasm_.get(),
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
      TestUtils::GetTestFileLists("core/runtime/vm/lepus/compiler/unit_test/");

  for (const auto& test_file : all_test_file) {
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx = lepus::VMContext();
    TestUtils::RegisterBuiltin(&vm_ctx);
    lepus::BytecodeGenerator::GenerateBytecode(&vm_ctx, src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(&vm_ctx);
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    auto binary_reader = LynxBinaryReaderTest(
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        false);
    ASSERT_TRUE(binary_reader.DecodeContextTest());
    std::shared_ptr<lepus::Context> decode_ctx =
        std::make_shared<lepus::VMContext>();
    auto entry = TemplateEntry(decode_ctx, target_sdk_version);
    TestUtils::RegisterBuiltin(decode_ctx.get());
    ASSERT_TRUE(decode_ctx->DeSerialize(
        *binary_reader.GetTemplateBundle().context_bundle_, true, nullptr));
    ASSERT_TRUE(decode_ctx->Execute());
  }
}

TEST_F(ContextBinaryReaderTest, LynxBinaryReaderLepusNG) {
  auto all_test_file = TestUtils::GetTestFileLists(
      "core/runtime/vm/lepus/compiler/lepusng_unit_test");

  for (const auto& test_file : all_test_file) {
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx = lepus::QuickContext();
    lepus::BytecodeGenerator::GenerateBytecode(&vm_ctx, src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(&vm_ctx);
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    auto binary_reader = LynxBinaryReaderTest(
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        true);
    ASSERT_TRUE(binary_reader.DecodeContextTest());
    std::shared_ptr<lepus::Context> decode_ctx =
        std::make_shared<lepus::QuickContext>();
    TestUtils::RegisterBuiltin(decode_ctx.get());
    auto entry = TemplateEntry(decode_ctx, target_sdk_version);
    ASSERT_TRUE(entry.GetVm()->DeSerialize(
        *binary_reader.GetTemplateBundle().context_bundle_, false, nullptr));
    ASSERT_TRUE(decode_ctx->Execute());
  }
}

// TemplateBinaryReader has the same context construction method as
// LynxBinaryReader, so disable the following two tests
TEST_F(ContextBinaryReaderTest, DISABLED_TemplateBinaryReaderLepus) {
  auto all_test_file =
      TestUtils::GetTestFileLists("core/runtime/vm/lepus/compiler/unit_test");

  for (const auto& test_file : all_test_file) {
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx = lepus::VMContext();
    TestUtils::RegisterBuiltin(&vm_ctx);
    lepus::BytecodeGenerator::GenerateBytecode(&vm_ctx, src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(&vm_ctx);
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    TemplateBinaryReaderTest binary_reader{
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        false};

    ASSERT_TRUE(binary_reader.DecodeContext());
    ASSERT_TRUE(binary_reader.GetVm()->Execute());
  }
}

TEST_F(ContextBinaryReaderTest, DISABLED_TemplateBinaryReaderLepusNG) {
  auto all_test_file = TestUtils::GetTestFileLists(
      "core/runtime/vm/lepus/compiler/lepusng_unit_test/");

  for (const auto& test_file : all_test_file) {
    std::cout << "[ContextDecoderTest] test file: " << test_file << std::endl;
    auto src = TestUtils::ReadFileFromPath(test_file);
    auto vm_ctx = lepus::QuickContext();
    TestUtils::RegisterBuiltin(&vm_ctx);
    lepus::BytecodeGenerator::GenerateBytecode(&vm_ctx, src,
                                               target_sdk_version);
    auto binary_writer = ContextBinaryWriterTest(&vm_ctx);
    binary_writer.encode();
    auto byte_array =
        const_cast<lepus::OutputStream*>(binary_writer.stream())->byte_array();

    TemplateBinaryReaderTest binary_reader{
        std::make_unique<lepus::ByteArrayInputStream>(std::move(byte_array)),
        true};

    ASSERT_TRUE(binary_reader.DecodeContext());
    ASSERT_TRUE(binary_reader.GetVm()->Execute());
  }
}

}  // namespace tasm
}  // namespace lynx
