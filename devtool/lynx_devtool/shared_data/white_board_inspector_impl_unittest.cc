// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/shared_data/white_board_inspector_impl.h"

#include "core/runtime/vm/lepus/json_parser.h"
#include "core/shared_data/lynx_white_board.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/testing/mock/white_board_inspector_delegate_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class WhiteBoardInspectorImplTest : public ::testing::Test {
 public:
  WhiteBoardInspectorImplTest() {}
  ~WhiteBoardInspectorImplTest() override {}
  void SetUp() override {
    inspector_ = std::make_shared<WhiteBoardInspectorImpl>();
    white_board_ = std::make_shared<tasm::WhiteBoard>();

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

 private:
  std::shared_ptr<WhiteBoardInspectorImpl> inspector_;
  std::shared_ptr<tasm::WhiteBoard> white_board_;
};

TEST_F(WhiteBoardInspectorImplTest, InsertDelegate) {
  auto delegate1 = std::make_shared<WhiteBoardInspectorDelegateMock>(1);
  inspector_->InsertDelegate(delegate1, 1);
  EXPECT_EQ(inspector_->delegates_.size(), 1);
  EXPECT_EQ(inspector_->delegates_[1].lock(), delegate1);

  auto delegate2 = std::make_shared<WhiteBoardInspectorDelegateMock>(1);
  inspector_->InsertDelegate(delegate2, 1);
  EXPECT_EQ(inspector_->delegates_.size(), 1);
  EXPECT_EQ(inspector_->delegates_[1].lock(), delegate2);

  auto delegate3 = std::make_shared<WhiteBoardInspectorDelegateMock>(2);
  inspector_->InsertDelegate(delegate3, 2);
  EXPECT_EQ(inspector_->delegates_.size(), 2);
  EXPECT_EQ(inspector_->delegates_[1].lock(), delegate2);
  EXPECT_EQ(inspector_->delegates_[2].lock(), delegate3);

  inspector_->RemoveDelegate(1);
  EXPECT_EQ(inspector_->delegates_.size(), 1);
  EXPECT_EQ(inspector_->delegates_.find(1), inspector_->delegates_.end());
}

TEST_F(WhiteBoardInspectorImplTest, SetSharedData) {
  int error_code = 0;
  std::string error_msg;

  std::string key1 = "key1";
  std::string value1 = "value1";
  inspector_->SetSharedData(key1, value1, error_code, error_msg);
  EXPECT_EQ(error_code, CDPErrorCode::kServerError);
  EXPECT_EQ(error_msg, "Failed to set shared data!");

  error_code = 0;
  error_msg.clear();

  inspector_->SetWhiteBoard(white_board_);
  std::string key2 = "key2";
  std::string value2 = "value2";
  inspector_->SetSharedData(key2, value2, error_code, error_msg);
  EXPECT_EQ(error_code, CDPErrorCode::kInvalidParams);
  EXPECT_EQ(error_msg, "The value must be a valid JSON string!");

  error_code = 0;
  error_msg.clear();

  // string value
  std::string key3 = "key3";
  std::string value3 = "\"value3\"";
  inspector_->SetSharedData(key3, value3, error_code, error_msg);
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(error_msg, "");
  auto result3 = white_board_->GetGlobalSharedData(key3);
  auto lepus_result3 = pub::ValueUtils::ConvertValueToLepusValue(*(result3));
  auto str_result3 = lepus::lepusValueToString(lepus_result3, true);
  EXPECT_EQ(str_result3, value3);

  // number value
  std::string key4 = "key4";
  std::string value4 = "123";
  inspector_->SetSharedData(key4, value4, error_code, error_msg);
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(error_msg, "");
  auto result4 = white_board_->GetGlobalSharedData(key4);
  auto lepus_result4 = pub::ValueUtils::ConvertValueToLepusValue(*(result4));
  auto str_result4 = lepus::lepusValueToString(lepus_result4, true);
  EXPECT_EQ(str_result4, value4);

  // object value
  std::string key5 = "key5";
  std::string value5 = "{\"number\":123,\"string\":\"123\"}";
  inspector_->SetSharedData(key5, value5, error_code, error_msg);
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(error_msg, "");
  auto result5 = white_board_->GetGlobalSharedData(key5);
  auto lepus_result5 = pub::ValueUtils::ConvertValueToLepusValue(*(result5));
  auto str_result5 = lepus::lepusValueToString(lepus_result5, true);
  EXPECT_EQ(str_result5, value5);

  // json string value
  std::string key6 = "key6";
  std::string value6 = "\"{\\\"number\\\":123,\\\"string\\\":\\\"123\\\"}\"";
  inspector_->SetSharedData(key6, value6, error_code, error_msg);
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(error_msg, "");
  auto result6 = white_board_->GetGlobalSharedData(key6);
  auto lepus_result6 = pub::ValueUtils::ConvertValueToLepusValue(*(result6));
  auto str_result6 = lepus::lepusValueToString(lepus_result6, true);
  EXPECT_EQ(str_result6, value6);
}

TEST_F(WhiteBoardInspectorImplTest, GetSharedData) {
  int error_code = 0;
  std::string error_msg;
  std::vector<std::pair<std::string, std::string>> result;

  inspector_->GetSharedData(result, error_code, error_msg);
  EXPECT_TRUE(result.empty());
  EXPECT_EQ(error_code, CDPErrorCode::kServerError);
  EXPECT_EQ(error_msg, "Failed to get shared data!");

  error_code = 0;
  error_msg.clear();
  inspector_->SetWhiteBoard(white_board_);

  inspector_->GetSharedData(result, error_code, error_msg);
  std::sort(result.begin(), result.end());
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(error_msg, "");
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].first, "key1");
  EXPECT_EQ(result[0].second, "\"value1\"");
  EXPECT_EQ(result[1].first, "key2");
  EXPECT_EQ(result[1].second, "\"value2\"");
}

TEST_F(WhiteBoardInspectorImplTest, RemoveSharedData) {
  int error_code = 0;
  std::string error_msg;
  std::string error_key = "error_key";

  inspector_->RemoveSharedData(error_key, error_code, error_msg);
  EXPECT_EQ(error_code, CDPErrorCode::kServerError);
  EXPECT_EQ(error_msg, "Failed to remove shared data!");

  error_code = 0;
  error_msg.clear();
  inspector_->SetWhiteBoard(white_board_);

  inspector_->RemoveSharedData(error_key, error_code, error_msg);
  EXPECT_EQ(error_code, CDPErrorCode::kInvalidParams);
  EXPECT_EQ(error_msg, "The key does not exist!");

  error_code = 0;
  error_msg.clear();

  inspector_->RemoveSharedData("key1", error_code, error_msg);
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(error_msg, "");
  EXPECT_EQ(white_board_->data_center_.size(), 1);
  EXPECT_EQ(white_board_->data_center_.find("key1"),
            white_board_->data_center_.end());
}

TEST_F(WhiteBoardInspectorImplTest, ClearSharedData) {
  int error_code = 0;
  std::string error_msg;

  inspector_->ClearSharedData(error_code, error_msg);
  EXPECT_EQ(error_code, CDPErrorCode::kServerError);
  EXPECT_EQ(error_msg, "Failed to clear shared data!");

  error_code = 0;
  error_msg.clear();
  inspector_->SetWhiteBoard(white_board_);

  inspector_->ClearSharedData(error_code, error_msg);
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(error_msg, "");
  EXPECT_TRUE(white_board_->data_center_.empty());
}

TEST_F(WhiteBoardInspectorImplTest, Notify) {
  auto delegate1 = std::make_shared<WhiteBoardInspectorDelegateMock>(1);
  inspector_->InsertDelegate(delegate1, 1);
  auto delegate2 = std::make_shared<WhiteBoardInspectorDelegateMock>(2);
  inspector_->InsertDelegate(delegate2, 2);
  delegate2->enabled_ = true;
  white_board_->SetInspector(inspector_);

  lepus::Value lepus_value = lepus::jsonValueTolepusValue("123");
  auto data = std::make_shared<pub::ValueImplLepus>(lepus_value);

  white_board_->SetGlobalSharedData("key3", data);
  EXPECT_TRUE(delegate1->event_message_.empty());
  EXPECT_NE(delegate2->event_message_.find("WhiteBoard.onSharedDataAdded"),
            std::string::npos);

  white_board_->SetGlobalSharedData("key1", data);
  EXPECT_TRUE(delegate1->event_message_.empty());
  EXPECT_NE(delegate2->event_message_.find("WhiteBoard.onSharedDataUpdated"),
            std::string::npos);

  white_board_->RemoveGlobalSharedData("key3");
  EXPECT_TRUE(delegate1->event_message_.empty());
  EXPECT_NE(delegate2->event_message_.find("WhiteBoard.onSharedDataRemoved"),
            std::string::npos);

  white_board_->ClearGlobalSharedData();
  EXPECT_TRUE(delegate1->event_message_.empty());
  EXPECT_NE(delegate2->event_message_.find("WhiteBoard.onSharedDataCleared"),
            std::string::npos);

  delegate2.reset();
  white_board_->SetGlobalSharedData("key4", data);
  EXPECT_EQ(inspector_->delegates_.size(), 1);
  EXPECT_EQ(inspector_->delegates_.find(2), inspector_->delegates_.end());
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
