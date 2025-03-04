// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/base_devtool/native/js_inspect/inspector_client_delegate_base_impl_unittest.h"

#include "base/include/fml/thread.h"
#include "core/base/json/json_util.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {
class InspectorClientDelegateBaseImplTest : public ::testing::Test {
 public:
  InspectorClientDelegateBaseImplTest() {}
  ~InspectorClientDelegateBaseImplTest() override {}
  void SetUp() override {
    v8_delegate_ =
        std::make_shared<MockInspectorClientDelegateBaseImpl>(kKeyEngineV8);
    qjs_delegate_ = std::make_shared<MockInspectorClientDelegateBaseImpl>(
        kKeyEngineQuickjs);
    lepus_delegate_ =
        std::make_shared<MockInspectorClientDelegateBaseImpl>(kKeyEngineLepus);
  }

 private:
  std::vector<double> repeating_timer_test_;
  std::shared_ptr<MockInspectorClientDelegateBaseImpl> v8_delegate_;
  std::shared_ptr<MockInspectorClientDelegateBaseImpl> qjs_delegate_;
  std::shared_ptr<MockInspectorClientDelegateBaseImpl> lepus_delegate_;
};

TEST_F(InspectorClientDelegateBaseImplTest, StartRepeatingTimer) {
  base::NoDestructor<lynx::fml::Thread> thread("test");
  thread->GetTaskRunner()->PostTask([delegate = v8_delegate_, test = this] {
    delegate->StartRepeatingTimer(
        0.1,
        [delegate](void* data) {
          auto* test =
              reinterpret_cast<InspectorClientDelegateBaseImplTest*>(data);
          test->repeating_timer_test_.emplace_back(delegate->CurrentTimeMS());
        },
        reinterpret_cast<void*>(test));
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  thread->GetTaskRunner()->PostTask([delegate = v8_delegate_, test = this] {
    delegate->CancelTimer(reinterpret_cast<void*>(test));
  });
  EXPECT_FALSE(repeating_timer_test_.empty());
}

// Only test the logic not covered by ScriptManagerNGTest.
TEST_F(InspectorClientDelegateBaseImplTest, CacheBreakpoints) {
  std::string active_message =
      "{\"id\":1,\"method\":\"Debugger.setBreakpointsActive\",\"params\":{"
      "\"active\":false}}";
  std::string enable_message = "{\"id\": 1, \"method\": \"Debugger.enable\"}";

  std::unique_ptr<ScriptManagerNG> script_manager;
  rapidjson::Document json_mes;

  // Test when script_manager is null.
  v8_delegate_->ParseStrToJson(json_mes, active_message);
  v8_delegate_->CacheBreakpointsByRequestMessage(json_mes, script_manager);
  v8_delegate_->CacheBreakpointsByResponseMessage(json_mes, script_manager);

  script_manager = std::make_unique<ScriptManagerNG>();
  v8_delegate_->CacheBreakpointsByRequestMessage(json_mes, script_manager);
  EXPECT_FALSE(script_manager->GetBreakpointsActive());

  json_mes.RemoveAllMembers();
  v8_delegate_->ParseStrToJson(json_mes, enable_message);
  v8_delegate_->CacheBreakpointsByRequestMessage(json_mes, script_manager);
  EXPECT_TRUE(script_manager->GetBreakpointsActive());
}

TEST_F(InspectorClientDelegateBaseImplTest, RecordDebuggingInstanceID) {
  std::string enable_message = "{\"id\": 1, \"method\": \"Debugger.enable\"}";
  std::string disable_message = "{\"id\": 2, \"method\": \"Debugger.disable\"}";
  rapidjson::Document json_mes;

  EXPECT_EQ(v8_delegate_->debugging_instance_id_, kErrorViewID);
  v8_delegate_->ParseStrToJson(json_mes, enable_message);
  v8_delegate_->RecordDebuggingInstanceID(json_mes, 1);
  EXPECT_EQ(v8_delegate_->debugging_instance_id_, 1);

  json_mes.RemoveAllMembers();
  v8_delegate_->ParseStrToJson(json_mes, disable_message);
  v8_delegate_->RecordDebuggingInstanceID(json_mes, 2);
  EXPECT_EQ(v8_delegate_->debugging_instance_id_, 1);

  json_mes.RemoveAllMembers();
  v8_delegate_->ParseStrToJson(json_mes, disable_message);
  v8_delegate_->RecordDebuggingInstanceID(json_mes, 1);
  EXPECT_EQ(v8_delegate_->debugging_instance_id_, kErrorViewID);
}

TEST_F(InspectorClientDelegateBaseImplTest, AddEngineTypeParam) {
  std::string original_message =
      "{\"id\":5,\"result\":{\"debuggerId\":\"("
      "A17FAA9AFC12486F54613F1ADA0961FC)\"}}";
  std::string v8_message =
      "{\"id\":5,\"result\":{\"debuggerId\":\"("
      "A17FAA9AFC12486F54613F1ADA0961FC)\",\"engineType\":\"V8\"}}";
  std::string qjs_message =
      "{\"id\":5,\"result\":{\"debuggerId\":\"("
      "A17FAA9AFC12486F54613F1ADA0961FC)\",\"engineType\":\"PrimJS\"}}";
  std::string error_message = "{\"id\":6,\"result\":{}}";

  rapidjson::Document json_mes;
  v8_delegate_->ParseStrToJson(json_mes, original_message);
  v8_delegate_->AddEngineTypeParam(json_mes);
  EXPECT_EQ(base::ToJson(json_mes), v8_message);

  json_mes.RemoveAllMembers();
  qjs_delegate_->ParseStrToJson(json_mes, original_message);
  qjs_delegate_->AddEngineTypeParam(json_mes);
  EXPECT_EQ(base::ToJson(json_mes), qjs_message);

  json_mes.RemoveAllMembers();
  lepus_delegate_->ParseStrToJson(json_mes, original_message);
  lepus_delegate_->AddEngineTypeParam(json_mes);
  EXPECT_EQ(base::ToJson(json_mes), original_message);

  json_mes.RemoveAllMembers();
  v8_delegate_->ParseStrToJson(json_mes, error_message);
  v8_delegate_->AddEngineTypeParam(json_mes);
  EXPECT_EQ(base::ToJson(json_mes), error_message);
}

TEST_F(InspectorClientDelegateBaseImplTest, GenSimpleMessage) {
  std::string expected = "{\"id\":1,\"method\":\"Debugger.enable\"}";
  std::string result = v8_delegate_->GenSimpleMessage("Debugger.enable", 1);
  EXPECT_EQ(result, expected);
}

TEST_F(InspectorClientDelegateBaseImplTest, GenMessageSetBreakpointByUrl) {
  std::string expected =
      "{\"id\":1,\"method\":\"Debugger.setBreakpointByUrl\",\"params\":{"
      "\"url\":\"test.js\",\"condition\":\"test\",\"lineNumber\":1,"
      "\"columnNumber\":1}}";
  std::string result =
      v8_delegate_->GenMessageSetBreakpointByUrl("test.js", "test", 1, 1, 1);
  EXPECT_EQ(result, expected);
}

TEST_F(InspectorClientDelegateBaseImplTest, GenMessageSetBreakpointsActive) {
  std::string expected =
      "{\"id\":1,\"method\":\"Debugger.setBreakpointsActive\",\"params\":{"
      "\"active\":true}}";
  std::string result = v8_delegate_->GenMessageSetBreakpointsActive(true, 1);
  EXPECT_EQ(result, expected);
}

TEST_F(InspectorClientDelegateBaseImplTest, GenMessageTargetCreated) {
  std::string expected =
      "{\"method\":\"Target.targetCreated\",\"params\":{\"targetInfo\":{"
      "\"targetId\":\"Lepus\",\"type\":\"worker\",\"title\":\"Lepus\",\"url\":"
      "\"\",\"attached\":false,\"canAccessOpener\":false}}}";
  std::string result =
      lepus_delegate_->GenMessageTargetCreated("Lepus", "Lepus");
  EXPECT_EQ(result, expected);
}

TEST_F(InspectorClientDelegateBaseImplTest, GenMessageAttachedToTarget) {
  std::string expected =
      "{\"method\":\"Target.attachedToTarget\",\"params\":{\"sessionId\":"
      "\"Lepus\",\"targetInfo\":{\"targetId\":\"Lepus\",\"type\":\"worker\","
      "\"title\":\"Lepus\",\"url\":\"\",\"attached\":true,\"canAccessOpener\":"
      "false},\"waitingForDebugger\":true}}";
  std::string result =
      lepus_delegate_->GenMessageAttachedToTarget("Lepus", "Lepus", "Lepus");
  EXPECT_EQ(result, expected);
}

TEST_F(InspectorClientDelegateBaseImplTest, GenMessageTargetDestroyed) {
  std::string expected =
      "{\"method\":\"Target.targetDestroyed\",\"params\":{\"targetId\":"
      "\"Lepus\"}}";
  std::string result = lepus_delegate_->GenMessageTargetDestroyed("Lepus");
  EXPECT_EQ(result, expected);
}

TEST_F(InspectorClientDelegateBaseImplTest, GenMessageDetachedFromTarget) {
  std::string expected =
      "{\"method\":\"Target.detachedFromTarget\",\"params\":{\"sessionId\":"
      "\"Lepus\"}}";
  std::string result = lepus_delegate_->GenMessageDetachedFromTarget("Lepus");
  EXPECT_EQ(result, expected);
}

TEST_F(InspectorClientDelegateBaseImplTest, ParseStrToJson) {
  std::string message = "{\"id\":1,\"method\":\"Debugger.enable\"}";
  std::string error_message =
      "{\"method\":\"Runtime.consoleAPICalled\",\"params\":{\"type\":\"log\","
      "\"args\":[{\"type\":\"string\",\"value\":\"runtimeId:1\"},{\"type\":"
      "\"string\",\"value\":\"test slice: "
      "\"},{\"type\":\"string\",\"value\":\"\\uD83D\"}],\"executionContextId\":"
      "1,\"timestamp\":408333,\"stackTrace\":{\"callFrames\":[{"
      "\"functionName\":\"logWithRuntimeId\",\"scriptId\":\"12\",\"url\":"
      "\"file://shared/"
      "lynx_core.js\",\"lineNumber\":8271,\"columnNumber\":35}]}}}";

  rapidjson::Document json_mes;
  EXPECT_TRUE(v8_delegate_->ParseStrToJson(json_mes, message));

  json_mes.RemoveAllMembers();
  EXPECT_FALSE(v8_delegate_->ParseStrToJson(json_mes, error_message));
}

TEST_F(InspectorClientDelegateBaseImplTest, RemoveInvalidMembers) {
  std::string message =
      "{\"id\":5,\"method\":\"Debugger.enable\",\"params\":{"
      "\"maxScriptsCacheSize\":100000000},\"timestamp\":1724070092831,"
      "\"sessionId\":\"SESSIONID\"}";
  std::string expected =
      "{\"id\":5,\"method\":\"Debugger.enable\",\"params\":{"
      "\"maxScriptsCacheSize\":100000000},\"sessionId\":\"SESSIONID\"}";

  rapidjson::Document json_mes;
  v8_delegate_->ParseStrToJson(json_mes, message);
  v8_delegate_->RemoveInvalidMembers(json_mes);
  EXPECT_EQ(base::ToJson(json_mes), expected);
}
}  // namespace testing
}  // namespace devtool
}  // namespace lynx
