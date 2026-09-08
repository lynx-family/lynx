// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/native_module_invocation_context.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/runtime/js/bindings/modules/native_module_record_builder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {

class CapturingNativeModuleRecordObserver : public NativeModuleRecordObserver {
 public:
  void OnRecord(const lepus::Value& record) override {
    records_.push_back(record);
  }

  std::vector<lepus::Value> records_;
};

TEST(NativeModuleValueSnapshotTest, PreservesNonFiniteNumbersAndBigInts) {
  auto input = lepus::Dictionary::Create();
  input->SetValue("nan", std::numeric_limits<double>::quiet_NaN());
  input->SetValue("positiveInfinity", std::numeric_limits<double>::infinity());
  input->SetValue("negativeInfinity", -std::numeric_limits<double>::infinity());
  input->SetValue("bigint", std::numeric_limits<int64_t>::max());

  bool truncated = false;
  lepus::Value output =
      SanitizeToProtocolValue(lepus::Value(std::move(input)), truncated);

  ASSERT_TRUE(output.IsTable());
  EXPECT_FALSE(truncated);
  EXPECT_EQ(
      output.Table()->GetValue("nan")->Table()->GetValue("$type")->StdString(),
      "number");
  EXPECT_EQ(
      output.Table()->GetValue("nan")->Table()->GetValue("value")->StdString(),
      "NaN");
  EXPECT_EQ(output.Table()
                ->GetValue("positiveInfinity")
                ->Table()
                ->GetValue("value")
                ->StdString(),
            "Infinity");
  EXPECT_EQ(output.Table()
                ->GetValue("negativeInfinity")
                ->Table()
                ->GetValue("value")
                ->StdString(),
            "-Infinity");
  EXPECT_EQ(output.Table()
                ->GetValue("bigint")
                ->Table()
                ->GetValue("$type")
                ->StdString(),
            "bigint");
  EXPECT_EQ(output.Table()
                ->GetValue("bigint")
                ->Table()
                ->GetValue("value")
                ->StdString(),
            "9223372036854775807");
}

TEST(NativeModuleValueSnapshotTest, AppliesSnapshotCaps) {
  auto input = lepus::CArray::Create();
  input->emplace_back("abcdef");
  input->emplace_back("second");

  ValueSnapshotCaps caps;
  caps.max_container_size = 1;
  caps.max_string_length = 3;
  bool truncated = false;
  lepus::Value output =
      SanitizeToProtocolValue(lepus::Value(std::move(input)), truncated, caps);

  ASSERT_TRUE(output.IsArray());
  ASSERT_EQ(output.Array()->size(), 1U);
  EXPECT_EQ(output.Array()->get(0).StdString(), "abc");
  EXPECT_TRUE(truncated);
}

TEST(NativeModuleInvocationContextTest,
     BuildsAndEmitsInvokeAndCallbackRecords) {
  auto observer = std::make_shared<CapturingNativeModuleRecordObserver>();
  NativeModuleInvocationContext invocation(observer, "LynxTestModule", "echo");

  auto arguments = lepus::CArray::Create();
  arguments->emplace_back("value");
  lepus::Value invoke_record = invocation.BuildInvokeRecord(
      lepus::Value(std::move(arguments)), true,
      lepus::Value(std::numeric_limits<double>::infinity()), 0, "");
  invocation.EmitRecord(invoke_record);

  ASSERT_TRUE(invoke_record.IsTable());
  auto invoke = invoke_record.Table();
  EXPECT_EQ(invoke->GetValue("method")->StdString(), "LynxTestModule.echo");
  EXPECT_EQ(invoke->GetValue("phase")->StdString(), "invoke");
  EXPECT_FALSE(invoke->GetValue("truncated")->Bool());
  ASSERT_TRUE(invoke->GetValue("arguments")->IsArray());
  EXPECT_EQ(invoke->GetValue("arguments")->Array()->get(0).StdString(),
            "value");
  EXPECT_EQ(invoke->GetValue("result")
                ->Table()
                ->GetValue("value")
                ->Table()
                ->GetValue("value")
                ->StdString(),
            "Infinity");

  auto callback = invocation.WithCallbackArgumentIndex(1);
  lepus::Value callback_record =
      callback->BuildCallbackRecord(lepus::Value(42));
  callback->EmitRecord(callback_record);

  ASSERT_EQ(observer->records_.size(), 2U);
  EXPECT_EQ(callback_record.Table()->GetValue("phase")->StdString(),
            "callback");
  EXPECT_EQ(callback_record.Table()->GetValue("callbackArgumentIndex")->Int32(),
            1);
  EXPECT_EQ(callback_record.Table()->GetValue("invocationId")->StdString(),
            invoke->GetValue("invocationId")->StdString());
  EXPECT_EQ(callback_record.Table()
                ->GetValue("result")
                ->Table()
                ->GetValue("value")
                ->Int32(),
            42);
}

TEST(NativeModuleValueSnapshotTest, TruncatesStringsAtUtf8Boundaries) {
  // ASCII, two-byte, three-byte and four-byte code points, followed by ASCII.
  const std::string input = "A\xC2\xA2\xE4\xB8\xAD\xF0\x9F\x98\x80Z";
  const std::vector<size_t> boundaries = {0, 1, 3, 6, 10, 11};
  for (size_t limit = 0; limit <= input.size() + 1; ++limit) {
    SCOPED_TRACE(limit);
    ValueSnapshotCaps caps;
    caps.max_string_length = limit;
    size_t expected_length = 0;
    for (size_t boundary : boundaries) {
      if (boundary <= limit) {
        expected_length = boundary;
      }
    }
    bool truncated = false;
    auto output = SanitizeToProtocolValue(lepus::Value(input), truncated, caps);
    ASSERT_TRUE(output.IsString());
    EXPECT_EQ(output.StdString(), input.substr(0, expected_length));
    EXPECT_EQ(truncated, limit < input.size());
  }

  ValueSnapshotCaps caps;
  caps.max_string_length = 0;
  bool truncated = false;
  auto output = SanitizeToProtocolValue(lepus::Value(""), truncated, caps);
  EXPECT_TRUE(output.StdString().empty());
  EXPECT_FALSE(truncated);
}

TEST(NativeModuleValueSnapshotTest, DefaultStringLimitPreservesUtf8) {
  const std::string prefix(8 * 1024 - 1, 'a');
  for (const std::string suffix : {"\xE4\xB8\xAD", "\xF0\x9F\x98\x80"}) {
    bool truncated = false;
    auto output =
        SanitizeToProtocolValue(lepus::Value(prefix + suffix), truncated);
    EXPECT_EQ(output.StdString(), prefix);
    EXPECT_TRUE(truncated);
  }
}

TEST(NativeModuleInvocationContextTest, PreservesExplicitNullResult) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto record =
      invocation.BuildInvokeRecord(lepus::Value(), true, lepus::Value(), 0, "");
  auto result = record.Table()->GetValue("result")->Table();
  EXPECT_TRUE(result->GetValue("success")->Bool());
  ASSERT_TRUE(result->Contains("value"));
  EXPECT_TRUE(result->GetValue("value")->IsNil());
}

TEST(NativeModuleInvocationContextTest, OmitsResultWhenNotCaptured) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto record =
      invocation.BuildInvokeRecord(lepus::Value(), true, std::nullopt, 0, "");
  auto result = record.Table()->GetValue("result")->Table();
  EXPECT_TRUE(result->GetValue("success")->Bool());
  EXPECT_FALSE(result->Contains("value"));
}

TEST(NativeModuleInvocationContextTest, PreservesExplicitUndefinedResult) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  lepus::Value undefined;
  undefined.SetUndefined();
  auto record =
      invocation.BuildInvokeRecord(lepus::Value(), true, undefined, 0, "");
  auto result = record.Table()->GetValue("result")->Table();
  ASSERT_TRUE(result->Contains("value"));
  ASSERT_TRUE(result->GetValue("value")->IsTable());
  EXPECT_EQ(result->GetValue("value")->Table()->GetValue("$type")->StdString(),
            "undefined");
  EXPECT_FALSE(record.Table()->GetValue("truncated")->Bool());
}

TEST(NativeModuleInvocationContextTest, PreservesNullCallbackArgument) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto callback = invocation.WithCallbackArgumentIndex(0);
  auto arguments = lepus::CArray::Create();
  arguments->emplace_back(lepus::Value());
  auto record = callback->BuildCallbackRecord(lepus::Value(arguments));
  auto result = record.Table()->GetValue("result")->Table();
  ASSERT_TRUE(result->GetValue("value")->IsArray());
  ASSERT_EQ(result->GetValue("value")->Array()->size(), 1U);
  EXPECT_TRUE(result->GetValue("value")->Array()->get(0).IsNil());
}

TEST(NativeModuleInvocationContextTest, FailedResultDoesNotExposeValue) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto record = invocation.BuildInvokeRecord(lepus::Value(), false,
                                             lepus::Value(), 42, "failure");
  auto result = record.Table()->GetValue("result")->Table();
  EXPECT_FALSE(result->GetValue("success")->Bool());
  EXPECT_FALSE(result->Contains("value"));
  EXPECT_EQ(result->GetValue("code")->Int32(), 42);
  EXPECT_EQ(result->GetValue("errMsg")->StdString(), "failure");
}

TEST(NativeModuleValueSnapshotTest, CopiesNestedContainers) {
  auto nested = lepus::Dictionary::Create();
  nested->SetValue("value", "original");
  auto input = lepus::CArray::Create();
  input->emplace_back(lepus::Value(nested));
  bool truncated = false;
  auto snapshot = SanitizeToProtocolValue(lepus::Value(input), truncated);

  nested->SetValue("value", "modified");
  input->emplace_back("new element");

  ASSERT_EQ(snapshot.Array()->size(), 1U);
  EXPECT_EQ(snapshot.Array()->get(0).Table()->GetValue("value")->StdString(),
            "original");
  EXPECT_FALSE(truncated);
}

}  // namespace
}  // namespace js
}  // namespace runtime
}  // namespace lynx
