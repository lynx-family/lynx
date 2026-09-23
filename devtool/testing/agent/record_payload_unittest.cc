// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/record_payload.h"

#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx::devtool {
namespace {
using tasm::recorder::SetObservationPayloadEnabled;
using tasm::recorder::SetRecordPayloadCallback;
class RecordPayloadEvent : public ::testing::Test {
 protected:
  void SetUp() override {
    previous_ =
        tasm::LynxEnv::GetInstance().GetBoolEnv("enable_devtool", false);
    tasm::LynxEnv::GetInstance().SetBoolLocalEnv("enable_devtool", true);
    tasm::DevToolLifecycle::GetInstance().SyncStateFromPlatform(
        tasm::DevToolState::CONNECTED);
    SetRecordPayloadCallback(GlobalDevToolPlatformFacade::SendRecordPayload);
    SetObservationPayloadEnabled(true);
    MockReceiver::GetInstance().ResetAll();
  }
  void TearDown() override {
    Drain();
    SetObservationPayloadEnabled(false);
    SetRecordPayloadCallback(nullptr);
    tasm::LynxEnv::GetInstance().SetBoolLocalEnv("enable_devtool", previous_);
    tasm::DevToolLifecycle::GetInstance().SyncStateFromPlatform(
        tasm::DevToolState::UNAVAILABLE);
  }
  void Drain() {
    runner_->PostSyncTask([] {});
  }
  std::string Send(std::string data) {
    return GlobalDevToolPlatformFacade::SendRecordPayload({1, 2, 3}, "0.0.1",
                                                          std::move(data));
  }
  bool previous_ = false;
  fml::RefPtr<fml::TaskRunner> runner_ =
      LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner();
};

TEST_F(RecordPayloadEvent, SendsWholeTextAndBinaryOnGlobalChannel) {
  for (auto data : {std::string{}, std::string(9 * 1024 * 1024, 'x'),
                    std::string("a\0\xff", 3)}) {
    auto id = Send(data);
    ASSERT_FALSE(id.empty());
    Drain();
    const auto& received = MockReceiver::GetInstance().received_message_;
    EXPECT_EQ(received.first, "CDP");
    Json::Value event;
    ASSERT_TRUE(Json::Reader().parse(received.second, event));
    EXPECT_EQ(event["method"].asString(), "Lynx.observePayload");
    const auto& params = event["params"];
    EXPECT_EQ(params["payloadId"].asString(), id);
    EXPECT_EQ(params["viewId"].asInt(), 1);
    EXPECT_EQ(params["bytes"].asUInt64(), data.size());
    auto decoded = params["data"].asString();
    if (params["encoding"].asString() == "base64") {
      std::string bytes(lynx_modp_b64_decode_len(decoded.size()), '\0');
      auto size =
          lynx_modp_b64_decode(bytes.data(), decoded.data(), decoded.size());
      ASSERT_NE(size, MODP_B64_ERROR);
      bytes.resize(size);
      decoded = std::move(bytes);
    } else {
      EXPECT_EQ(params["encoding"].asString(), "utf8");
    }
    EXPECT_EQ(decoded, data);
  }
}

TEST_F(RecordPayloadEvent, DropsDisabledOrDisconnectedData) {
  runner_->PostSyncTask([&] {
    EXPECT_FALSE(Send(std::string(6 * 1024 * 1024, 'x')).empty());
    EXPECT_FALSE(Send(std::string(6 * 1024 * 1024, 'y')).empty());
    SetObservationPayloadEnabled(false);
  });
  Drain();
  EXPECT_TRUE(MockReceiver::GetInstance().received_message_.second.empty());
  EXPECT_TRUE(Send("disabled").empty());
  SetObservationPayloadEnabled(true);
  runner_->PostSyncTask([&] {
    EXPECT_FALSE(Send("queued").empty());
    tasm::DevToolLifecycle::GetInstance().OnDisconnected();
  });
  Drain();
  EXPECT_TRUE(MockReceiver::GetInstance().received_message_.second.empty());
  EXPECT_TRUE(Send("disconnected").empty());
}
}  // namespace
}  // namespace lynx::devtool
