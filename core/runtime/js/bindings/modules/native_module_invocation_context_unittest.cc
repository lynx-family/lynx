// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/native_module_invocation_context.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/public/jsb/lynx_module_callback.h"
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

TEST(NativeModuleInvocationContextTest,
     BuildsAndEmitsInvokeAndCallbackRecords) {
  auto observer = std::make_shared<CapturingNativeModuleRecordObserver>();
  NativeModuleInvocationContext invocation(observer, "LynxTestModule", "echo");

  auto arguments = lepus::CArray::Create();
  arguments->emplace_back("value");
  lepus::Value invoke_record = invocation.BuildInvokeRecord(
      lepus::Value(std::move(arguments)), CallbackMap{}, true,
      lepus::Value(int32_t{7}), 0, "");
  invocation.EmitRecord(invoke_record);

  ASSERT_TRUE(invoke_record.IsTable());
  auto invoke = invoke_record.Table();
  EXPECT_EQ(invoke->GetValue("method")->StdString(), "LynxTestModule.echo");
  EXPECT_EQ(invoke->GetValue("phase")->StdString(), "invoke");
  EXPECT_EQ(invoke->GetValue("type")->StdString(), "call");
  ASSERT_TRUE(invoke->GetValue("arguments")->IsArray());
  EXPECT_EQ(invoke->GetValue("arguments")->Array()->get(0).StdString(),
            "value");
  EXPECT_TRUE(invoke->GetValue("result")->Table()->GetValue("success")->Bool());
  EXPECT_EQ(invoke->GetValue("result")->Table()->GetValue("value")->Int32(), 7);

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

TEST(NativeModuleInvocationContextTest, PreservesExplicitNullResult) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto record = invocation.BuildInvokeRecord(lepus::Value(), CallbackMap{},
                                             true, lepus::Value(), 0, "");
  auto result = record.Table()->GetValue("result")->Table();
  EXPECT_TRUE(result->GetValue("success")->Bool());
  ASSERT_TRUE(result->Contains("value"));
  EXPECT_TRUE(result->GetValue("value")->IsNil());
}

TEST(NativeModuleInvocationContextTest, OmitsResultWhenNotCaptured) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto record = invocation.BuildInvokeRecord(lepus::Value(), CallbackMap{},
                                             true, std::nullopt, 0, "");
  auto result = record.Table()->GetValue("result")->Table();
  EXPECT_TRUE(result->GetValue("success")->Bool());
  EXPECT_FALSE(result->Contains("value"));
}

TEST(NativeModuleInvocationContextTest, FailedResultDoesNotExposeValue) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto record = invocation.BuildInvokeRecord(
      lepus::Value(), CallbackMap{}, false, lepus::Value(), 42, "failure");
  auto result = record.Table()->GetValue("result")->Table();
  EXPECT_FALSE(result->GetValue("success")->Bool());
  EXPECT_FALSE(result->Contains("value"));
  EXPECT_EQ(result->GetValue("code")->Int32(), 42);
  EXPECT_EQ(result->GetValue("errMsg")->StdString(), "failure");
}

TEST(NativeModuleInvocationContextTest,
     CallbackContextInheritsInvocationIdentity) {
  auto observer = std::make_shared<CapturingNativeModuleRecordObserver>();
  NativeModuleInvocationContext invocation(observer, "LynxTestModule", "echo");
  auto callback = invocation.WithCallbackArgumentIndex(2);
  EXPECT_EQ(callback->invocation_id(), invocation.invocation_id());
  EXPECT_EQ(callback->module_name(), invocation.module_name());
  EXPECT_EQ(callback->method_name(), invocation.method_name());
  EXPECT_EQ(callback->callback_argument_index(), 2);
}

TEST(NativeModuleInvocationContextTest, EmitRecordIsNoopWithoutObserver) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto record = invocation.BuildInvokeRecord(lepus::Value(), CallbackMap{},
                                             true, std::nullopt, 0, "");
  invocation.EmitRecord(record);
}

TEST(NativeModuleInvocationContextTest,
     ReplacesCallbackArgumentsWithPlaceholder) {
  NativeModuleInvocationContext invocation({}, "LynxTestModule", "echo");
  auto arguments = lepus::CArray::Create();
  arguments->emplace_back("value");
  arguments->emplace_back(int64_t{1024});
  CallbackMap callbacks;
  callbacks.emplace(1, nullptr);

  auto record = invocation.BuildInvokeRecord(
      lepus::Value(std::move(arguments)), callbacks, true, std::nullopt, 0, "");
  ASSERT_TRUE(record.Table()->GetValue("arguments")->IsArray());
  auto args = record.Table()->GetValue("arguments")->Array();
  ASSERT_EQ(args->size(), 2U);
  EXPECT_EQ(args->get(0).StdString(), "value");
  ASSERT_TRUE(args->get(1).IsTable());
  auto placeholder = args->get(1).Table();
  EXPECT_EQ(placeholder->GetValue("$type")->StdString(), "callback");
  EXPECT_EQ(placeholder->GetValue("argumentIndex")->Int32(), 1);
}

TEST(NativeModuleRecordBuilderTest, BuildsGlobalEventRecord) {
  auto arguments = lepus::CArray::Create();
  arguments->emplace_back("payload");
  auto record =
      BuildGlobalEventRecord("customEvent", lepus::Value(std::move(arguments)));
  ASSERT_TRUE(record.IsTable());
  auto table = record.Table();
  EXPECT_EQ(table->GetValue("type")->StdString(), "event");
  EXPECT_EQ(table->GetValue("method")->StdString(), "customEvent");
  ASSERT_TRUE(table->GetValue("arguments")->IsArray());
  ASSERT_EQ(table->GetValue("arguments")->Array()->size(), 1U);
  EXPECT_EQ(
      table->GetValue("arguments")->Array()->get(0).Array()->get(0).StdString(),
      "payload");
}

TEST(NativeModuleRecordBuilderTest, BuildsCallbackPlaceholder) {
  auto placeholder = BuildCallbackPlaceholder(3);
  ASSERT_TRUE(placeholder.IsTable());
  auto table = placeholder.Table();
  EXPECT_EQ(table->GetValue("$type")->StdString(), "callback");
  EXPECT_EQ(table->GetValue("argumentIndex")->Int32(), 3);
}

}  // namespace
}  // namespace js
}  // namespace runtime
}  // namespace lynx
