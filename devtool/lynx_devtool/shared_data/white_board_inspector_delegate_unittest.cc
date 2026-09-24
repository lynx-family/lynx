// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/shared_data/white_board_inspector_delegate.h"

#include <memory>
#include <string>

#include "core/runtime/lepus/json_parser.h"
#include "core/shared_data/lynx_white_board.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/testing/mock/white_board_inspector_delegate_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class WhiteBoardInspectorDelegateTest : public ::testing::Test {
 public:
  WhiteBoardInspectorDelegateTest() {}
  ~WhiteBoardInspectorDelegateTest() override {}
  void SetUp() override {
    MockReceiver::GetInstance().ResetAll();
    delegate_ = std::make_shared<WhiteBoardInspectorDelegateMock>(1);
    inspector_ = std::make_shared<WhiteBoardInspectorImpl>();
    white_board_ = std::make_shared<tasm::WhiteBoard>();
    inspector_->SetWhiteBoard(white_board_);
    white_board_->SetInspector(inspector_);
    message_sender_ = std::make_shared<MessageSenderMock>();

    std::string key1 = "key1";
    std::string value1 = "\"value1\"";
    std::string key2 = "key2";
    std::string value2 = "\"value2\"";

    lepus::Value lepus_value1 = lepus::jsonValueTolepusValue(value1.c_str());
    auto data1 = std::make_shared<pub::ValueImplLepus>(lepus_value1);
    white_board_->SetGlobalSharedData(key1, data1);
    lepus::Value lepus_value2 = lepus::jsonValueTolepusValue(value2.c_str());
    auto data2 = std::make_shared<pub::ValueImplLepus>(lepus_value2);
    white_board_->SetGlobalSharedData(key2, data2);
  }

 protected:
  // Runs a delegate command handler through a fresh CDPResponder and returns
  // the parsed CDP envelope the responder emitted.
  Json::Value RunCommand(void (WhiteBoardInspectorDelegate::*method)(
                             const std::shared_ptr<CDPResponder>&,
                             const Json::Value&),
                         const Json::Value& params, int64_t id = 123) {
    MockReceiver::GetInstance().ResetAll();
    auto responder = std::make_shared<CDPResponder>(message_sender_, id);
    (delegate_.get()->*method)(responder, params);
    responder.reset();
    Json::Value response;
    Json::Reader reader;
    reader.parse(MockReceiver::GetInstance().received_message_.second, response,
                 false);
    return response;
  }

  std::shared_ptr<WhiteBoardInspectorDelegateMock> delegate_;
  std::shared_ptr<WhiteBoardInspectorImpl> inspector_;
  std::shared_ptr<tasm::WhiteBoard> white_board_;
  std::shared_ptr<MessageSenderMock> message_sender_;
};

TEST_F(WhiteBoardInspectorDelegateTest, Enable) {
  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::Enable, Json::Value(), 123);
  EXPECT_EQ(response["id"].asInt64(), 123);
  EXPECT_TRUE(response["result"].isObject());
  EXPECT_TRUE(response["result"].empty());
  EXPECT_TRUE(delegate_->enabled_);
}

TEST_F(WhiteBoardInspectorDelegateTest, Disable) {
  delegate_->enabled_ = true;
  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::Disable, Json::Value(), 123);
  EXPECT_EQ(response["id"].asInt64(), 123);
  EXPECT_TRUE(response["result"].isObject());
  EXPECT_FALSE(delegate_->enabled_);
}

TEST_F(WhiteBoardInspectorDelegateTest, SetSharedDataWhenDisabledReturnsError) {
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "key3";
  params["value"] = "value3";

  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::SetSharedData, params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "WhiteBoard is not enabled");
}

TEST_F(WhiteBoardInspectorDelegateTest,
       SetSharedDataWithoutInspectorReturnsError) {
  delegate_->enabled_ = true;
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "key3";
  params["value"] = "\"value3\"";

  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::SetSharedData, params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "WhiteBoard inspector is unavailable");
}

TEST_F(WhiteBoardInspectorDelegateTest, SetSharedDataRejectsInvalidJson) {
  delegate_->enabled_ = true;
  delegate_->SetInspector(inspector_);
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "key3";
  params["value"] = "value3";

  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::SetSharedData, params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
  EXPECT_EQ(response["error"]["message"].asString(),
            "The value must be a valid JSON string!");
}

TEST_F(WhiteBoardInspectorDelegateTest, SetSharedDataSucceeds) {
  delegate_->enabled_ = true;
  delegate_->SetInspector(inspector_);
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "key3";
  params["value"] = "\"value3\"";

  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::SetSharedData, params);
  EXPECT_EQ(response["id"].asInt64(), 123);
  EXPECT_TRUE(response["result"].isObject());
}

TEST_F(WhiteBoardInspectorDelegateTest, GetSharedDataWhenDisabledReturnsError) {
  Json::Value response = RunCommand(&WhiteBoardInspectorDelegate::GetSharedData,
                                    Json::Value(Json::ValueType::objectValue));
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "WhiteBoard is not enabled");
}

TEST_F(WhiteBoardInspectorDelegateTest, GetSharedDataReportsBackendFailure) {
  delegate_->enabled_ = true;
  delegate_->SetInspector(inspector_);
  inspector_->white_board_.reset();

  Json::Value response = RunCommand(&WhiteBoardInspectorDelegate::GetSharedData,
                                    Json::Value(Json::ValueType::objectValue));
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Failed to get shared data!");
}

TEST_F(WhiteBoardInspectorDelegateTest, GetSharedDataReturnsEntries) {
  delegate_->enabled_ = true;
  delegate_->SetInspector(inspector_);

  Json::Value response = RunCommand(&WhiteBoardInspectorDelegate::GetSharedData,
                                    Json::Value(Json::ValueType::objectValue));
  EXPECT_EQ(response["id"].asInt64(), 123);
  ASSERT_TRUE(response["result"]["entries"].isArray());
  EXPECT_EQ(response["result"]["entries"].size(), 2u);
}

TEST_F(WhiteBoardInspectorDelegateTest,
       RemoveSharedDataWhenDisabledReturnsError) {
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "key1";

  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::RemoveSharedData, params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "WhiteBoard is not enabled");
}

TEST_F(WhiteBoardInspectorDelegateTest, RemoveSharedDataRejectsMissingKey) {
  delegate_->enabled_ = true;
  delegate_->SetInspector(inspector_);
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "key3";

  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::RemoveSharedData, params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
  EXPECT_EQ(response["error"]["message"].asString(), "The key does not exist!");
}

TEST_F(WhiteBoardInspectorDelegateTest, RemoveSharedDataSucceeds) {
  delegate_->enabled_ = true;
  delegate_->SetInspector(inspector_);
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "key1";

  Json::Value response =
      RunCommand(&WhiteBoardInspectorDelegate::RemoveSharedData, params);
  EXPECT_EQ(response["id"].asInt64(), 123);
  EXPECT_TRUE(response["result"].isObject());
}

TEST_F(WhiteBoardInspectorDelegateTest, ClearReportsBackendFailure) {
  delegate_->enabled_ = true;
  delegate_->SetInspector(inspector_);
  inspector_->white_board_.reset();

  Json::Value response = RunCommand(&WhiteBoardInspectorDelegate::Clear,
                                    Json::Value(Json::ValueType::objectValue));
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Failed to clear shared data!");
}

TEST_F(WhiteBoardInspectorDelegateTest, ClearSucceeds) {
  delegate_->enabled_ = true;
  delegate_->SetInspector(inspector_);

  Json::Value response = RunCommand(&WhiteBoardInspectorDelegate::Clear,
                                    Json::Value(Json::ValueType::objectValue));
  EXPECT_EQ(response["id"].asInt64(), 123);
  EXPECT_TRUE(response["result"].isObject());
}

TEST_F(WhiteBoardInspectorDelegateTest, Notify) {
  std::string key = "key";
  std::string value = "123";
  std::string expected;

  delegate_->OnSharedDataAdded(key, value);
  expected =
      "{\n   \"method\" : \"WhiteBoard.onSharedDataAdded\",\n   \"params\" : "
      "{\n      \"key\" : \"key\",\n      \"value\" : \"123\"\n   }\n}\n";
  EXPECT_EQ(delegate_->event_message_, expected);

  delegate_->OnSharedDataUpdated(key, value);
  expected =
      "{\n   \"method\" : \"WhiteBoard.onSharedDataUpdated\",\n   \"params\" : "
      "{\n      \"key\" : \"key\",\n      \"newValue\" : \"123\"\n   }\n}\n";
  EXPECT_EQ(delegate_->event_message_, expected);

  delegate_->OnSharedDataRemoved(key);
  expected =
      "{\n   \"method\" : \"WhiteBoard.onSharedDataRemoved\",\n   \"params\" : "
      "{\n      \"key\" : \"key\"\n   }\n}\n";
  EXPECT_EQ(delegate_->event_message_, expected);

  delegate_->OnSharedDataCleared();
  expected = "{\n   \"method\" : \"WhiteBoard.onSharedDataCleared\"\n}\n";
  EXPECT_EQ(delegate_->event_message_, expected);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
