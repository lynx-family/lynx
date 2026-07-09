// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"

#include <sys/wait.h>

#include <cstddef>
#include <memory>
#include <vector>

#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/jsoncpp/include/json/value.h"

namespace lynx {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
static constexpr size_t kBoxModelSize = 34;
static constexpr size_t kTransformValueSize = 32;

static std::vector<float> BuildTransformValue(float start) {
  std::vector<float> transform_value(kTransformValueSize);
  for (size_t i = 0; i < transform_value.size(); ++i) {
    transform_value[i] = start + static_cast<float>(i);
  }
  return transform_value;
}

class InspectorUIExecutorTest : public ::testing::Test {
 public:
  InspectorUIExecutorTest() = default;
  ~InspectorUIExecutorTest() override {}

  void SetUp() override {
    lynx::tasm::LynxEnvConfig lynx_env_config(
        kWidth, kHeight, kDefaultLayoutsUnitPerPx,
        kDefaultPhysicalPixelsPerLayoutUnit);
    devtool::MockReceiver::GetInstance().ResetAll();
    devtool_mediator_ = std::make_shared<lynx::devtool::LynxDevToolMediator>();
    devtools_ng_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    message_sender_ = std::make_shared<devtool::MessageSenderMock>();
    devtools_ng_->message_sender_ = message_sender_;
    devtool_mediator_->devtool_wp_ = devtools_ng_;
    ui_executor_ =
        std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
    ui_thread_ = std::make_unique<fml::Thread>("ui");
    devtool_mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
  }

 private:
  std::shared_ptr<devtool::InspectorUIExecutor> ui_executor_;
  std::shared_ptr<devtool::LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<devtool::MessageSender> message_sender_;
  std::shared_ptr<testing::LynxDevToolNGMock> devtools_ng_;
  std::unique_ptr<fml::Thread> ui_thread_;
};

TEST_F(InspectorUIExecutorTest, PageReloadTest) {
  LOGI("InspectorUIExecutorTest PageReloadTest start");

  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  ui_executor_->SetDevToolPlatformFacade(facade);
  EXPECT_EQ(ui_executor_->devtool_platform_facade_.get(), facade.get());
  {
    // test empty value
    Json::Value message;
    message["id"] = 21;

    ui_executor_->PageReload(message_sender_, message);

    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 21,\n   \"result\" : {}\n}\n");
  }

  {
    // test partial value
    Json::Value message;
    message["id"] = 22;
    Json::Value params;
    params["ignoreCache"] = false;
    message["params"] = params;
    ui_executor_->PageReload(message_sender_, message);

    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 22,\n   \"result\" : {}\n}\n");
  }

  {
    // test normal value
    Json::Value message;
    message["id"] = 23;
    Json::Value params;
    params["ignoreCache"] = true;
    params["pageData"] = "test_template_binary_data";
    params["fromPageDataFragments"] = true;
    params["pageDataLength"] = 2048;
    params["url"] = "http://test.example.com/reload";
    message["params"] = params;
    ui_executor_->PageReload(message_sender_, message);

    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 23,\n   \"result\" : {}\n}\n");
  }
}

TEST_F(InspectorUIExecutorTest, StartScreencastTest) {
  LOGI("InspectorUIExecutorTest StartScreencastTest start");

  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  ui_executor_->SetDevToolPlatformFacade(facade);
  EXPECT_EQ(ui_executor_->devtool_platform_facade_.get(), facade.get());

  {
    Json::Value message;
    message["id"] = 31;
    ui_executor_->StartScreencast(message_sender_, message);
    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 31,\n   \"result\" : {}\n}\n");
  }

  {
    Json::Value message;
    message["id"] = 32;
    Json::Value params;
    params["format"] = "jpeg";
    params["quality"] = 80;
    message["params"] = params;
    ui_executor_->StartScreencast(message_sender_, message);
    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 32,\n   \"result\" : {}\n}\n");
  }

  {
    Json::Value message;
    message["id"] = 33;
    Json::Value params;
    params["format"] = "png";
    params["quality"] = 100;
    params["maxWidth"] = 720;
    params["maxHeight"] = 1280;
    params["everyNthFrame"] = 2;
    params["mode"] = "lynxview";
    message["params"] = params;
    ui_executor_->StartScreencast(message_sender_, message);
    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 33,\n   \"result\" : {}\n}\n");
  }
}

TEST_F(InspectorUIExecutorTest, InsertTextTest) {
  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  ui_executor_->SetDevToolPlatformFacade(facade);

  Json::Value message;
  message["id"] = 41;
  Json::Value params;
  params["text"] = "hello";
  message["params"] = params;

  ui_executor_->InsertText(message_sender_, message);

  EXPECT_EQ(facade->inserted_text_, "hello");
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 41,\n   \"result\" : {}\n}\n");
}

TEST_F(InspectorUIExecutorTest, GetBoxModelReturnsOverlaySnapshotCase) {
  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->InitWithDevToolMediator(devtool_mediator_);

  std::vector<double> overlay_box_model(kBoxModelSize);
  for (size_t i = 0; i < overlay_box_model.size(); ++i) {
    overlay_box_model[i] = static_cast<double>(i);
  }

  devtool::InspectorBoxModelQuery query;
  query.is_overlay = true;
  query.overlay_box_model = overlay_box_model;

  EXPECT_EQ(facade->GetBoxModel(query), overlay_box_model);
  EXPECT_TRUE(facade->transform_value_ids_.empty());
}

TEST_F(InspectorUIExecutorTest, GetBoxModelUsesLayoutOnlyOffsetSnapshotCase) {
  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->InitWithDevToolMediator(devtool_mediator_);
  facade->transform_value_response_ = BuildTransformValue(100.f);

  devtool::InspectorBoxModelQuery query;
  query.has_ui_primitive = false;
  query.layout_object.id = 11;
  query.layout_object.has_snapshot = true;
  query.layout_object.border_bound_width = 50;
  query.layout_object.border_bound_height = 20;
  query.layout_object.layout_padding_left = 1;
  query.layout_object.layout_padding_top = 2;
  query.layout_object.layout_padding_right = 3;
  query.layout_object.layout_padding_bottom = 4;
  query.layout_object.layout_border_left_width = 5;
  query.layout_object.layout_border_top_width = 6;
  query.layout_object.layout_border_right_width = 7;
  query.layout_object.layout_border_bottom_width = 8;
  query.layout_object.layout_margin_left = 9;
  query.layout_object.layout_margin_top = 10;
  query.layout_object.layout_margin_right = 11;
  query.layout_object.layout_margin_bottom = 12;

  devtool::InspectorLayoutObjectInfo layout_only_node;
  layout_only_node.id = 12;
  layout_only_node.has_snapshot = true;
  layout_only_node.border_bound_left_from_parent_padding_bound = 4;
  layout_only_node.border_bound_top_from_parent_padding_bound = 6;
  query.layout_only_nodes.push_back(layout_only_node);

  query.transform_node.id = 13;
  query.transform_node.has_snapshot = true;
  query.transform_node.border_bound_width = 200;
  query.transform_node.border_bound_height = 100;
  query.transform_node.layout_border_left_width = 2;
  query.transform_node.layout_border_top_width = 3;

  std::vector<double> box_model = facade->GetBoxModel(query);

  ASSERT_EQ(box_model.size(), kBoxModelSize);
  EXPECT_DOUBLE_EQ(box_model[0], 34);
  EXPECT_DOUBLE_EQ(box_model[1], 0);
  ASSERT_EQ(facade->transform_value_ids_.size(), 1U);
  EXPECT_EQ(facade->transform_value_ids_[0], 13);
  ASSERT_EQ(facade->transform_value_inputs_.size(), 1U);
  const std::vector<float>& pad_border_margin_layout =
      facade->transform_value_inputs_[0];
  ASSERT_EQ(pad_border_margin_layout.size(), 16U);
  EXPECT_FLOAT_EQ(pad_border_margin_layout[0], 1.f);
  EXPECT_FLOAT_EQ(pad_border_margin_layout[11], 12.f);
  EXPECT_FLOAT_EQ(pad_border_margin_layout[12], 6.f);
  EXPECT_FLOAT_EQ(pad_border_margin_layout[13], 9.f);
  EXPECT_FLOAT_EQ(pad_border_margin_layout[14], 144.f);
  EXPECT_FLOAT_EQ(pad_border_margin_layout[15], 71.f);
  EXPECT_DOUBLE_EQ(box_model[2], 100);
  EXPECT_DOUBLE_EQ(box_model[33], 131);
}

TEST_F(InspectorUIExecutorTest, GetBoxModelForwardsQueryToPlatformFacadeCase) {
  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->box_model_response_ = std::vector<double>(kBoxModelSize, 7);
  ui_executor_->SetDevToolPlatformFacade(facade);

  devtool::InspectorBoxModelQuery query;
  query.layout_object.id = 21;
  query.transform_node.id = 22;
  query.has_ui_primitive = true;

  EXPECT_EQ(ui_executor_->GetBoxModel(query), facade->box_model_response_);
  ASSERT_EQ(facade->box_model_queries_.size(), 1U);
  EXPECT_EQ(facade->box_model_queries_[0].layout_object.id, 21);
  EXPECT_EQ(facade->box_model_queries_[0].transform_node.id, 22);
}

}  // namespace testing
}  // namespace lynx
