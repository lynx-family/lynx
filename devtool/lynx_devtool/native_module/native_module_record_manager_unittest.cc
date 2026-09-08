// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/jsoncpp/include/json/value.h"
#define private public
#define protected public

#include <cstdint>
#include <memory>
#include <string>

#include "base/include/value/table.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/native_module/native_module_record_manager.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace {

lepus::Value MakeRecord(int32_t index) {
  auto record = lepus::Dictionary::Create();
  record->SetValue("invocationId", std::to_string(index));
  record->SetValue("method", "LynxTestModule.echo");
  record->SetValue("phase", "invoke");
  return lepus::Value(std::move(record));
}

Json::Value ReceivedMessage() {
  Json::Value message;
  Json::Reader reader;
  EXPECT_TRUE(reader.parse(MockReceiver::GetInstance().received_message_.second,
                           message));
  return message;
}

class NativeModuleRecordManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    MockReceiver::GetInstance().ResetAll();
    mediator_ = std::make_shared<LynxDevToolMediator>();
    devtool_ = std::make_shared<testing::LynxDevToolNGMock>();
    sender_ = std::make_shared<MessageSenderMock>();
    devtool_->message_sender_ = sender_;
    devtool_->devtool_mediator_ = mediator_;
    mediator_->devtool_wp_ = devtool_;
    manager_ = std::make_shared<NativeModuleRecordManager>(mediator_);
  }

  void AddRecord(int32_t index) {
    manager_->EnqueueRecordOnJSThread(MakeRecord(index));
    manager_->DrainPendingQueue();
  }

  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<testing::LynxDevToolNGMock> devtool_;
  std::shared_ptr<MessageSender> sender_;
  std::shared_ptr<NativeModuleRecordManager> manager_;
};

TEST_F(NativeModuleRecordManagerTest, EnableDisableAndReplayHistory) {
  AddRecord(1);
  manager_->GetRecords(sender_, 7);

  Json::Value response = ReceivedMessage();
  ASSERT_EQ(response["id"].asInt64(), 7);
  ASSERT_EQ(response["result"]["latestSequence"].asInt64(), 1);
  ASSERT_EQ(response["result"]["records"].size(), 1U);
  EXPECT_EQ(response["result"]["records"][0]["sequence"].asInt64(), 1);
  EXPECT_EQ(response["result"]["records"][0]["method"].asString(),
            "LynxTestModule.echo");

  manager_->Enable();
  MockReceiver::GetInstance().ResetAll();
  AddRecord(2);
  Json::Value event = ReceivedMessage();
  EXPECT_EQ(event["method"].asString(), "LynxNativeModule.recordAdded");
  EXPECT_EQ(event["params"]["record"]["sequence"].asInt64(), 2);

  manager_->Disable();
  MockReceiver::GetInstance().ResetAll();
  AddRecord(3);
  EXPECT_TRUE(MockReceiver::GetInstance().received_message_.second.empty());

  manager_->GetRecords(sender_, 8);
  response = ReceivedMessage();
  EXPECT_EQ(response["id"].asInt64(), 8);
  EXPECT_EQ(response["result"]["latestSequence"].asInt64(), 3);
  EXPECT_EQ(response["result"]["records"].size(), 3U);
}

TEST_F(NativeModuleRecordManagerTest, EvictsOldestHistoryRecord) {
  for (size_t i = 1; i <= NativeModuleRecordManager::kMaxHistoryCount + 1;
       ++i) {
    AddRecord(static_cast<int32_t>(i));
  }

  manager_->GetRecords(sender_, 1);
  Json::Value response = ReceivedMessage();
  const Json::Value& records = response["result"]["records"];
  ASSERT_EQ(records.size(), NativeModuleRecordManager::kMaxHistoryCount);
  EXPECT_EQ(records[0]["sequence"].asInt64(), 2);
  EXPECT_EQ(records[records.size() - 1]["sequence"].asInt64(), 201);
  EXPECT_EQ(response["result"]["latestSequence"].asInt64(), 201);
}

TEST_F(NativeModuleRecordManagerTest, DropsNewestWhenPendingQueueIsFull) {
  // Pretend a drain is already scheduled so enqueues don't post their own drain
  // onto the DevTool thread; this lets the pending queue fill deterministically
  // instead of being consumed mid-loop.
  manager_->drain_scheduled_ = true;
  for (size_t i = 0; i < NativeModuleRecordManager::kMaxPendingCount; ++i) {
    manager_->EnqueueRecordOnJSThread(MakeRecord(static_cast<int32_t>(i)));
  }
  manager_->EnqueueRecordOnJSThread(MakeRecord(1024));

  EXPECT_EQ(manager_->dropped_count(), 1);
  manager_->DrainPendingQueue();
  EXPECT_EQ(manager_->latest_sequence(),
            static_cast<int64_t>(NativeModuleRecordManager::kMaxPendingCount));
}

}  // namespace
}  // namespace devtool
}  // namespace lynx
