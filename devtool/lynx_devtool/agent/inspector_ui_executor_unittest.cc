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

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
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

class ImmediateTaskRunner : public fml::TaskRunner {
 public:
  ImmediateTaskRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
};

class TestDevToolPlatformFacade : public DevToolPlatformFacadeMock {
 public:
  using DevToolPlatformFacade::GetBoxModelInGeneralPlatform;

  std::vector<double> GetBoxModel(tasm::Element* element) override {
    get_box_model_element_ = element;
    return box_model_;
  }

  std::vector<float> GetTransformValue(
      int identifier,
      const std::vector<float>& pad_border_margin_layout) override {
    transform_identifier_ = identifier;
    pad_border_margin_layout_ = pad_border_margin_layout;
    return transform_value_;
  }

  tasm::Element* get_box_model_element_{nullptr};
  int transform_identifier_{-1};
  std::vector<double> box_model_;
  std::vector<float> transform_value_;
  std::vector<float> pad_border_margin_layout_;
};

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

  std::unique_ptr<tasm::ElementManager> CreateElementManager() {
    tasm::LynxEnvConfig lynx_env_config(kWidth, kHeight,
                                        kDefaultLayoutsUnitPerPx,
                                        kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_delegate_ =
        std::make_shared<::testing::NiceMock<tasm::test::MockTasmDelegate>>();
    auto manager = std::make_unique<tasm::ElementManager>(
        std::make_unique<tasm::MockPaintingContext>(), tasm_delegate_.get(),
        lynx_env_config);
    auto config = std::make_shared<tasm::PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
    return manager;
  }

  fml::RefPtr<tasm::Element> CreateView(tasm::ElementManager* manager,
                                        const std::string& tag = "view") {
    return manager->CreateFiberElement(tag);
  }

  std::shared_ptr<SLNode> CreateLayoutObject() {
    if (!layout_computed_css_) {
      layout_computed_css_ =
          std::make_unique<starlight::ComputedCSSStyle>(1.f, 1.f);
    }
    return std::make_shared<SLNode>(
        layout_configs_, layout_computed_css_->GetLayoutComputedStyle());
  }

 private:
  std::shared_ptr<devtool::InspectorUIExecutor> ui_executor_;
  std::shared_ptr<devtool::LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<devtool::MessageSender> message_sender_;
  std::shared_ptr<testing::LynxDevToolNGMock> devtools_ng_;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::shared_ptr<::testing::NiceMock<tasm::test::MockTasmDelegate>>
      tasm_delegate_;
  starlight::LayoutConfigs layout_configs_;
  std::unique_ptr<starlight::ComputedCSSStyle> layout_computed_css_;
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

TEST_F(InspectorUIExecutorTest,
       GetBoxModelInGeneralPlatformUsesNonWrapperParentForWrapperElement) {
  auto manager = CreateElementManager();
  auto root = CreateView(manager.get());
  auto wrapper_parent = CreateView(manager.get(), "wrapper");
  auto wrapper_child = CreateView(manager.get(), "wrapper");
  root->InsertNode(wrapper_parent);
  wrapper_parent->InsertNode(wrapper_child);

  auto facade = std::make_shared<TestDevToolPlatformFacade>();
  facade->box_model_ = {1.0, 2.0, 3.0};
  ui_executor_->SetDevToolPlatformFacade(facade);
  devtool_mediator_->ui_executor_ = ui_executor_;
  devtool_mediator_->ui_task_runner_ =
      fml::MakeRefCounted<ImmediateTaskRunner>();
  facade->InitWithDevToolMediator(devtool_mediator_);

  auto result = facade->GetBoxModelInGeneralPlatform(wrapper_child.get());

  EXPECT_EQ(result, facade->box_model_);
  EXPECT_EQ(facade->get_box_model_element_, root.get());
}

TEST_F(InspectorUIExecutorTest,
       GetBoxModelInGeneralPlatformSkipsWrapperParentForLayoutOnlyElement) {
  auto manager = CreateElementManager();
  auto root = CreateView(manager.get());
  auto wrapper_parent = CreateView(manager.get(), "wrapper");
  auto layout_only_child = CreateView(manager.get());
  root->InsertNode(wrapper_parent);
  wrapper_parent->InsertNode(layout_only_child);
  root->MarkCanBeLayoutOnly(false);
  layout_only_child->MarkCanBeLayoutOnly(true);
  layout_only_child->has_layout_only_props_ = true;
  layout_only_child->computed_css_style()->SetOverflowDefaultVisible(true);

  auto child_layout = CreateLayoutObject();
  child_layout->SetBorderBoundWidth(20.f);
  child_layout->SetBorderBoundHeight(10.f);
  child_layout->SetBorderBoundLeftFromParentPaddingBound(4.f);
  child_layout->SetBorderBoundTopFromParentPaddingBound(6.f);
  auto root_layout = CreateLayoutObject();
  root_layout->SetBorderBoundWidth(100.f);
  root_layout->SetBorderBoundHeight(80.f);

  auto facade = std::make_shared<TestDevToolPlatformFacade>();
  facade->transform_value_ = {7.f, 8.f};
  ui_executor_->SetDevToolPlatformFacade(facade);
  devtool_mediator_->ui_executor_ = ui_executor_;
  devtool_mediator_->ui_task_runner_ =
      fml::MakeRefCounted<ImmediateTaskRunner>();
  ui_executor_->layout_objects_[layout_only_child->impl_id()] =
      child_layout.get();
  ui_executor_->layout_objects_[root->impl_id()] = root_layout.get();
  facade->InitWithDevToolMediator(devtool_mediator_);

  auto result = facade->GetBoxModelInGeneralPlatform(layout_only_child.get());

  EXPECT_EQ(facade->transform_identifier_, root->impl_id());
  ASSERT_GE(facade->pad_border_margin_layout_.size(), 16U);
  EXPECT_EQ(facade->pad_border_margin_layout_[12], 4.f);
  EXPECT_EQ(facade->pad_border_margin_layout_[13], 6.f);
  ASSERT_EQ(result.size(), 4U);
  EXPECT_EQ(result[0], 20.0);
  EXPECT_EQ(result[1], 10.0);
  EXPECT_EQ(result[2], 7.0);
  EXPECT_EQ(result[3], 8.0);
}

}  // namespace testing
}  // namespace lynx
