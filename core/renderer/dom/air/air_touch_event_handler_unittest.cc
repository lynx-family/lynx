// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public

#include "core/renderer/dom/air/air_touch_event_handler.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/air/air_element/air_element.h"
#include "core/renderer/dom/air/air_element/air_page_element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class AirTouchEventHandlerTest : public ::testing::Test {
 public:
  AirTouchEventHandlerTest() {}
  ~AirTouchEventHandlerTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  std::unique_ptr<AirTouchEventHandler> air_touch_event_handler;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    manager->SetConfig(config);

    air_touch_event_handler =
        std::make_unique<AirTouchEventHandler>(manager->air_node_manager());
  }

  fml::RefPtr<AirLepusRef> CreateAirNode(const base::String &tag,
                                         int32_t lepus_id) {
    std::shared_ptr<AirElement> element =
        std::make_shared<AirElement>(kAirNormal, manager.get(), tag, lepus_id);
    manager->air_node_manager()->Record(element->impl_id(), element);
    manager->air_node_manager()->RecordForLepusId(
        lepus_id, static_cast<uint64_t>(lepus_id),
        AirLepusRef::Create(element));
    return AirLepusRef::Create(element);
  }
};

TEST_F(AirTouchEventHandlerTest, GenerateResponseChain) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  CSSValue css_value(lepus::Value("visible"),
                     lynx::tasm::CSSValuePattern::STRING);
  parent->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value);

  auto element1 = CreateAirNode("view", lepus_id++)->Get();
  element1->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value);
  parent->InsertNode(element1);

  auto element2 = CreateAirNode("view", lepus_id++)->Get();
  element2->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value);
  element1->InsertNode(element2);

  auto tag = element2->impl_id();

  EventOption bubble_option = {.bubbles_ = true,
                               .composed_ = false,
                               .capture_phase_ = false,
                               .lepus_event_ = false,
                               .from_frontend_ = false};

  auto bubble_chain =
      air_touch_event_handler->GenerateResponseChain(tag, bubble_option);
  EXPECT_EQ(static_cast<int>(bubble_chain.size()), 3);

  EventOption non_bubble_option = {.bubbles_ = false,
                                   .composed_ = false,
                                   .capture_phase_ = false,
                                   .lepus_event_ = false,
                                   .from_frontend_ = false};
  auto non_bubble_chain =
      air_touch_event_handler->GenerateResponseChain(tag, non_bubble_option);
  EXPECT_EQ(static_cast<int>(non_bubble_chain.size()), 1);
}

TEST_F(AirTouchEventHandlerTest, GetTargetInfo) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  auto target_info = air_touch_event_handler->GetTargetInfo(parent);

  EXPECT_TRUE(target_info.Table()->Contains("dataset"));
  EXPECT_TRUE(target_info.Table()->Contains("uid"));
}

TEST_F(AirTouchEventHandlerTest, GetEventHandler) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);

  auto element = CreateAirNode("view", 1)->Get();
  element->SetEventHandler(
      "tap", element->SetEvent("bindEvent", "tap", "clickProductCover"));

  auto event_handler = air_touch_event_handler->GetEventHandler(element, "tap");
  EXPECT_TRUE(event_handler != nullptr);
}

TEST_F(AirTouchEventHandlerTest, GetCustomEventParam) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  EventOption option;
  auto custom_event_param = air_touch_event_handler->GetCustomEventParam(
      "load", "detail", option, parent, parent, lepus_value());

  EXPECT_TRUE(custom_event_param.Table()->Contains("type"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("timestamp"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("currentTarget"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("target"));
}

TEST_F(AirTouchEventHandlerTest, GetTouchEventParam) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  auto custom_event_param = air_touch_event_handler->GetTouchEventParam(
      "tap", parent, parent, 289, 279, 289, 279, 289, 279);

  EXPECT_TRUE(custom_event_param.Table()->Contains("type"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("timestamp"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("currentTarget"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("target"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("detail"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("touches"));
  EXPECT_TRUE(custom_event_param.Table()->Contains("changedTouches"));
}

TEST_F(AirTouchEventHandlerTest, TriggerComponentEvent) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();

  CSSValue style(lepus::Value("visible"), lynx::tasm::CSSValuePattern::STRING);
  parent->SetStyle(CSSPropertyID::kPropertyIDOverflow, style);

  auto element1 = CreateAirNode("view", lepus_id++)->Get();
  element1->SetStyle(CSSPropertyID::kPropertyIDOverflow, style);
  element1->SetEventHandler(
      "myEvent", element1->SetEvent("bindEvent", "myEvent", "myEventClosure"));
  parent->InsertNode(element1);

  auto element2 = CreateAirNode("view", lepus_id++)->Get();
  element2->SetStyle(CSSPropertyID::kPropertyIDOverflow, style);
  element1->InsertNode(element2);
  auto element3 = CreateAirNode("view", lepus_id++)->Get();
  element3->SetStyle(CSSPropertyID::kPropertyIDOverflow, style);
  element2->InsertNode(element3);
  auto detail = lepus::Value(lynx::lepus::Dictionary::Create());
  detail.SetProperty(base::String("prop1"), lepus::Value("value1"));
  detail.SetProperty("componentId",
                     lepus::Value(static_cast<int64_t>(element3->impl_id())));
  size_t count = air_touch_event_handler->TriggerComponentEvent(
      nullptr, "myEvent", detail);
  EXPECT_EQ(count, 1);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
