// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/events/touch_event_handler.h"

#include "core/renderer/dom/vdom/radon/radon_dispatch_option.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

class TouchEventHandlerTest : public ::testing::Test {
 protected:
  TouchEventHandlerTest();
  ~TouchEventHandlerTest() {}

  void SetUp();

  void TearDown();

  std::unique_ptr<MockTasmDelegate> delegate_;
  std::unique_ptr<TouchEventHandler> touch_event_handler_;
  std::shared_ptr<TemplateAssembler> tasm_;
};

TouchEventHandlerTest::TouchEventHandlerTest() {}

void TouchEventHandlerTest::SetUp() {
  static constexpr int32_t kWidth = 1024;
  static constexpr int32_t kHeight = 768;
  static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
  static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

  auto lynx_env_config =
      LynxEnvConfig(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                    kDefaultPhysicalPixelsPerLayoutUnit);
  delegate_ = std::make_unique<MockTasmDelegate>();

  auto manager =
      std::make_unique<ElementManager>(std::make_unique<MockPaintingContext>(),
                                       delegate_.get(), lynx_env_config);

  touch_event_handler_ = std::make_unique<TouchEventHandler>(
      manager->node_manager(), *delegate_, true, true, "2.12");
  tasm_ = std::make_shared<lynx::tasm::TemplateAssembler>(
      *delegate_, std::move(manager), 0);
}

void TouchEventHandlerTest::TearDown() {}

TEST_F(TouchEventHandlerTest, TestGetCustomEventParamName) {
  EventOption option = {.bubbles_ = false,
                        .composed_ = false,
                        .capture_phase_ = false,
                        .lepus_event_ = false,
                        .from_frontend_ = false};
  base::String tag("view");
  auto element =
      tasm_->page_proxy()->element_manager()->CreateFiberElement(tag);
  lepus::Value params = lepus::Value::CreateObject();
  lepus::Value res1 = touch_event_handler_->GetCustomEventParam(
      "xxx", "detail", option, element.get(), element.get(), params, false);
  EXPECT_EQ(res1.IsObject(), true);
  EXPECT_EQ(res1.Table().get()->Contains("detail"), true);
  EXPECT_EQ(!res1.Table().get()->Contains("params"), true);
  lepus::Value res2 = touch_event_handler_->GetCustomEventParam(
      "xxx", "params", option, element.get(), element.get(), params, false);
  EXPECT_EQ(res2.IsObject(), true);
  EXPECT_EQ(res2.Table().get()->Contains("detail"), true);
  EXPECT_EQ(res2.Table().get()->Contains("params"), true);
}

TEST_F(TouchEventHandlerTest, TestHandleTriggerComponentEvent0) {
  touch_event_handler_->HandleTriggerComponentEvent(nullptr, "xxxx",
                                                    lepus::Value());
  EXPECT_EQ(delegate_->DumpDelegate(), "");
}

TEST_F(TouchEventHandlerTest, TestHandleTriggerComponentEvent1) {
  touch_event_handler_->HandleTriggerComponentEvent(
      tasm_.get(), "xxxx", lepus::Value::CreateObject());
  EXPECT_EQ(delegate_->DumpDelegate(), "");
}

TEST_F(TouchEventHandlerTest, TestHandleTriggerComponentEvent2) {
  lepus::Value obj = lepus::Value::CreateObject();
  obj.SetProperty("componentId", lepus::Value("1"));
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
  EXPECT_EQ(delegate_->DumpDelegate(), "");

  auto component = std::make_unique<RadonComponent>(
      tasm_->page_proxy(), 1, nullptr, nullptr, nullptr, nullptr, 1);
  tasm_->page_proxy()->component_map_[1] = component.get();
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
  EXPECT_EQ(delegate_->DumpDelegate(), "");

  tasm_->page_config_ = std::make_shared<PageConfig>();
  tasm_->page_config_->need_remove_component_element_ = true;
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
  EXPECT_EQ(delegate_->DumpDelegate(), "");

  tasm_->page_config_->need_remove_component_element_ = false;
  DispatchOption option(tasm_->page_proxy());
  component->SetStaticEvent("bindEvent", "xxxx", "onXXXX");
  component->RadonNode::DispatchSelf(option);
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
  EXPECT_EQ(delegate_->DumpDelegate(),
            "PublishComponentEvent 0 onXXXX "
            "{\"currentTarget\":{\"dataset\":{},\"id\":\"\"},\"detail\":null,"
            "\"target\":{\"dataset\":{},\"id\":\"\"},\"type\":\"xxxx\"}\n");
}

TEST_F(TouchEventHandlerTest, TestHandleTriggerComponentEvent3) {
  lepus::Value obj = lepus::Value::CreateObject();
  obj.SetProperty("componentId", lepus::Value("1"));
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
  EXPECT_EQ(delegate_->DumpDelegate(), "");

  // Create a unique pointer to a RadonComponent and add it to the component map
  auto component = std::make_unique<RadonComponent>(
      tasm_->page_proxy(), 1, nullptr, nullptr, nullptr, nullptr, 1);
  tasm_->page_proxy()->component_map_[1] = component.get();

  // Handle trigger component event and verify the delegate's dump
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
  EXPECT_EQ(delegate_->DumpDelegate(), "");

  DispatchOption option(tasm_->page_proxy());

  // Set up a fiber component and its worklet event handler, then handle trigger
  // component event
  tasm_->page_config_ = std::make_shared<PageConfig>();
  tasm_->page_config_->enable_fiber_arch_ = true;
  base::String component_id("1");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto fiber_component =
      tasm_->page_proxy()->element_manager()->CreateFiberComponent(
          component_id, css_id, entry_name, component_name, path);
  fiber_component->SetWorkletEventHandler("xxxx", "bindEvent", lepus::Value(),
                                          nullptr);
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
}

TEST_F(TouchEventHandlerTest, TestHandlerTriggerGestureEvent) {
  lepus::Value obj = lepus::Value::CreateObject();
  obj.SetProperty("componentId", lepus::Value("1"));
  touch_event_handler_->HandleGestureEvent(tasm_.get(), "xxxx", 1, 1, obj);
  EXPECT_EQ(delegate_->DumpDelegate(), "");

  // Create a unique pointer to a RadonComponent and add it to the component map
  auto component = std::make_unique<RadonComponent>(
      tasm_->page_proxy(), 1, nullptr, nullptr, nullptr, nullptr, 1);
  tasm_->page_proxy()->component_map_[1] = component.get();

  // Handle trigger component event and verify the delegate's dump
  touch_event_handler_->HandleGestureEvent(tasm_.get(), "xxxx", 1, 1, obj);
  EXPECT_EQ(delegate_->DumpDelegate(), "");

  DispatchOption option(tasm_->page_proxy());

  // Set up a fiber component and its worklet event handler, then handle trigger
  // component event
  tasm_->page_config_ = std::make_shared<PageConfig>();
  tasm_->page_config_->enable_fiber_arch_ = true;
  base::String component_id("1");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto fiber_component =
      tasm_->page_proxy()->element_manager()->CreateFiberComponent(
          component_id, css_id, entry_name, component_name, path);
  fiber_component->SetGestureDetector(
      1, GestureDetector(
             1, GestureType::PAN,
             {GestureCallback("xxxx", lepus::Value(), lepus::Value())}, {}));
  touch_event_handler_->HandleGestureEvent(tasm_.get(), "xxxx", 10, 1, obj);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
