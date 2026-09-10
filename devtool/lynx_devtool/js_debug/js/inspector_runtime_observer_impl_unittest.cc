// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <chrono>
#include <future>

#define protected public
#define private public

#include "base/include/fml/thread.h"
#include "base/include/value/array.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/runtime/js/bindings/modules/native_module_record_builder.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/lynx_devtool/agent/inspector_default_executor.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_runtime_observer_impl.h"
#include "devtool/lynx_devtool/js_debug/js/runtime_manager_delegate_impl.h"
#include "devtool/testing/mock/lynx_devtool_mediator_mock.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorRuntimeObserverImplTest : public ::testing::Test {
 public:
  InspectorRuntimeObserverImplTest() {}
  ~InspectorRuntimeObserverImplTest() override {}
  void SetUp() override {
    devtool_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    const auto& mediator = devtool_->devtool_mediator_;
    debugger_ = std::make_shared<InspectorJavaScriptDebuggerImpl>(mediator, 1);
    observer_ = debugger_->GetInspectorRuntimeObserver();
    observer_->SetDevToolMediator(mediator);
    auto devtool_executor =
        std::make_shared<InspectorDefaultExecutor>(mediator);
    mediator->devtool_executor_ = devtool_executor;
    mediator->devtool_wp_ = devtool_;
    auto message_sender = std::make_shared<devtool::MessageSenderMock>();
    devtool_->message_sender_ = message_sender;
  }

 private:
  std::shared_ptr<InspectorRuntimeObserverImpl> observer_;
  std::shared_ptr<InspectorJavaScriptDebuggerImpl> debugger_;
  std::shared_ptr<lynx::testing::LynxDevToolNGMock> devtool_;
};

TEST_F(InspectorRuntimeObserverImplTest, CreateRuntimeManagerDelegate) {
  auto runtime_manager_delegate = observer_->CreateRuntimeManagerDelegate();
  EXPECT_NE(runtime_manager_delegate, nullptr);
}

TEST_F(InspectorRuntimeObserverImplTest, NativeModuleRecordReachesHistory) {
  const auto& mediator = devtool_->devtool_mediator_;
  mediator->native_module_record_manager_ =
      std::make_shared<NativeModuleRecordManager>(mediator);
  auto record_observer = observer_->CreateNativeModuleRecordObserver();
  ASSERT_NE(record_observer, nullptr);

  std::promise<void> done;
  auto completed = done.get_future();
  fml::Thread devtool_thread("devtool");
  fml::Thread js_thread("js");
  mediator->default_task_runner_ = devtool_thread.GetTaskRunner();
  MockReceiver::GetInstance().ResetAll();
  js_thread.GetTaskRunner()->PostTask([&, record_observer] {
    auto arguments = lepus::CArray::Create();
    arguments->emplace_back("payload");
    auto record = runtime::js::BuildInvokeRecord(
        1, "LynxTestModule", "echo", lepus::Value(std::move(arguments)), true,
        lepus::Value("payload"), 0, "");
    record_observer->OnRecord(record);

    Json::Value request;
    request["id"] = 7;
    mediator->NativeModuleGetRecords(devtool_->message_sender_, request);
    mediator->RunOnDevToolThread([&done] { done.set_value(); });
  });
  ASSERT_EQ(completed.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);

  const auto& received = MockReceiver::GetInstance().received_message_;
  EXPECT_EQ(received.first, "CDP");
  Json::Value response;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(received.second, response));
  EXPECT_EQ(response["id"].asInt(), 7);
  EXPECT_EQ(response["result"]["latestSequence"].asInt64(), 1);
  const auto& records = response["result"]["records"];
  ASSERT_EQ(records.size(), 1U);
  EXPECT_EQ(records[0]["sequence"].asInt64(), 1);
  EXPECT_EQ(records[0]["invocationId"].asString(), "1");
  EXPECT_EQ(records[0]["method"].asString(), "LynxTestModule.echo");
  EXPECT_EQ(records[0]["type"].asString(), "call");
  EXPECT_EQ(records[0]["phase"].asString(), "invoke");
  ASSERT_EQ(records[0]["arguments"].size(), 1U);
  EXPECT_EQ(records[0]["arguments"][0].asString(), "payload");
  EXPECT_TRUE(records[0]["result"]["success"].asBool());
  EXPECT_EQ(records[0]["result"]["value"].asString(), "payload");
}

TEST_F(InspectorRuntimeObserverImplTest, OnRuntimeCreated) {
  devtool_->devtool_mediator_->devtool_executor_->console_msg_manager_
      ->enable_ = true;

  observer_->OnRuntimeCreated(runtime::js::JSRuntimeType::v8);
  sleep(1);
  std::string result1 = MockReceiver::GetInstance().received_message_.second;
  Json::Value json_res1;
  Json::Reader reader1;
  reader1.parse(result1, json_res1, false);
  std::string level1 = json_res1["params"]["entry"]["level"].asString();
  std::string text1 = json_res1["params"]["entry"]["text"].asString();
  std::string expected_level1 = "info";
  std::string expected_text1 = "[LynxDevTool] Current BTS engine type: V8";
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
  EXPECT_EQ(level1, expected_level1);
  EXPECT_EQ(text1, expected_text1);

  MockReceiver::GetInstance().received_message_ = {"", ""};
  observer_->OnRuntimeCreated(runtime::js::JSRuntimeType::jsc);
  sleep(1);
  std::string result2 = MockReceiver::GetInstance().received_message_.second;
  Json::Value json_res2;
  Json::Reader reader2;
  reader2.parse(result2, json_res2, false);
  std::string level2 = json_res2["params"]["entry"]["level"].asString();
  std::string text2 = json_res2["params"]["entry"]["text"].asString();
  std::string expected_level2 = "info";
  std::string expected_text2 = "[LynxDevTool] Current BTS engine type: JSC";
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
  EXPECT_EQ(level2, expected_level2);
  EXPECT_EQ(text2, expected_text2);

  MockReceiver::GetInstance().received_message_ = {"", ""};
  observer_->OnRuntimeCreated(runtime::js::JSRuntimeType::quickjs);
  sleep(1);
  std::string result3 = MockReceiver::GetInstance().received_message_.second;
  Json::Value json_res3;
  Json::Reader reader3;
  reader3.parse(result3, json_res3, false);
  std::string level3 = json_res3["params"]["entry"]["level"].asString();
  std::string text3 = json_res3["params"]["entry"]["text"].asString();
  std::string expected_level3 = "info";
  std::string expected_text3 = "[LynxDevTool] Current BTS engine type: PrimJS";
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
  EXPECT_EQ(level3, expected_level3);
  EXPECT_EQ(text3, expected_text3);

  MockReceiver::GetInstance().received_message_ = {"", ""};
  observer_->OnRuntimeCreated(runtime::js::JSRuntimeType::jsvm);
  sleep(1);
  std::string result4 = MockReceiver::GetInstance().received_message_.second;
  Json::Value json_res4;
  Json::Reader reader4;
  reader4.parse(result4, json_res4, false);
  std::string level4 = json_res4["params"]["entry"]["level"].asString();
  std::string text4 = json_res4["params"]["entry"]["text"].asString();
  std::string expected_level4 = "info";
  std::string expected_text4 = "[LynxDevTool] Current BTS engine type: JSVM";
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
  EXPECT_EQ(level4, expected_level4);
  EXPECT_EQ(text4, expected_text4);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
