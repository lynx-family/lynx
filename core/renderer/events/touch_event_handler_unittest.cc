// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/events/touch_event_handler.h"

#include "base/include/value/base_value.h"
#include "core/renderer/dom/vdom/radon/radon_dispatch_option.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/runtime/js/runtime_constant.h"
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
  std::unique_ptr<TemplateAssembler> tasm_;
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
  tasm_ = std::make_unique<lynx::tasm::TemplateAssembler>(
      *delegate_, std::move(manager), delegate_.get(), 0);
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
  lepus::Value params(lepus::Dictionary::Create());
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

TEST_F(TouchEventHandlerTest, TestGetTargetInfoNodeIndex) {
  auto holder = fml::MakeRefCounted<AttributeHolder>();
  holder->SetIdSelector(base::String("target"));

  lepus::Value target_info_without_element =
      TouchEventHandler::GetTargetInfo(11, holder.get());
  ASSERT_TRUE(target_info_without_element.IsObject());
  EXPECT_FALSE(target_info_without_element.Table()->Contains("nodeIndex"));

  base::String tag("view");
  auto element =
      tasm_->page_proxy()->element_manager()->CreateFiberElement(tag);
  element->SetNodeIndex(42);
  auto page_config = std::make_shared<PageConfig>();
  tasm_->page_proxy()->element_manager()->SetConfig(page_config);

  lepus::Value target_info_without_switch = TouchEventHandler::GetTargetInfo(
      element->impl_id(), element->data_model(), element.get());
  ASSERT_TRUE(target_info_without_switch.IsObject());
  EXPECT_FALSE(target_info_without_switch.Table()->Contains("nodeIndex"));

  page_config->SetEnableEventTargetInfoNodeIndex(true);
  lepus::Value target_info_with_element = TouchEventHandler::GetTargetInfo(
      element->impl_id(), element->data_model(), element.get());
  ASSERT_TRUE(target_info_with_element.IsObject());
  EXPECT_EQ(target_info_with_element.Table()->GetValue("nodeIndex").Number(),
            42);
}

TEST_F(TouchEventHandlerTest, SendGlobalEventToCoreContext) {
  auto* native_context_proxy =
      tasm_->GetContextProxy(runtime::ContextProxy::Type::kNative);
  ASSERT_NE(native_context_proxy, nullptr);

  int32_t event_count = 0;
  lepus::Value received_args;
  native_context_proxy->AddEventListener(
      runtime::kMessageEventTypeGlobalEvent,
      std::make_shared<event::ClosureEventListener>(
          [&event_count, &received_args](lepus::Value args) {
            ++event_count;
            received_args = lepus_value::ShallowCopy(args);
          }));

  lepus::Value info(lepus::Dictionary::Create());
  info.SetProperty("key", lepus::Value("value"));
  touch_event_handler_->SendGlobalEvent(tasm_.get(), EventType::kComponent,
                                        "global-event", info);

  EXPECT_EQ(event_count, 1);
  ASSERT_TRUE(received_args.IsArray());
  ASSERT_EQ(received_args.Array()->size(), 2U);
  EXPECT_EQ(received_args.Array()->get(0), lepus::Value("global-event"));
  const auto& received_params = received_args.Array()->get(1);
  ASSERT_TRUE(received_params.IsArray());
  ASSERT_EQ(received_params.Array()->size(), 1U);
  EXPECT_EQ(received_params.Array()->get(0), info);
}

TEST_F(TouchEventHandlerTest, HandleBubbleInputEventWithRefactoredDispatcher) {
  auto page_config = std::make_shared<PageConfig>();
  page_config->SetEnableEventHandleRefactor(true);
  tasm_->page_config_ = page_config;
  tasm_->page_proxy()->element_manager()->SetConfig(page_config);

  auto element =
      tasm_->page_proxy()->element_manager()->CreateFiberElement("view");
  element->MarkAttached();
  int call_count = 0;
  lepus::Value received_detail;
  element->AddEventListener(
      "pointerdown", std::make_shared<event::ClosureEventListener>(
                         [&call_count, &received_detail](lepus::Value args) {
                           ++call_count;
                           received_detail =
                               lepus::Value::Clone(args.Array()->get(1));
                         }));

  auto params = lepus::Dictionary::Create();
  params->SetValue("pointerId", 3);
  params->SetValue("pointerType", "mouse");
  touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", "pointerdown",
                                          element->impl_id(), params);

  EXPECT_EQ(call_count, 1);
  ASSERT_TRUE(received_detail.IsTable());
  EXPECT_EQ(received_detail.Table()->GetValue("pointerId").Number(), 3);
  EXPECT_EQ(received_detail.Table()->GetValue("pointerType").StdString(),
            "mouse");
  EXPECT_EQ(received_detail.Table()->GetValue("type").StdString(),
            "pointerdown");

  int key_count = 0;
  lepus::Value received_key_detail;
  element->AddEventListener(
      "keydown", std::make_shared<event::ClosureEventListener>(
                     [&key_count, &received_key_detail](lepus::Value args) {
                       ++key_count;
                       received_key_detail =
                           lepus::Value::Clone(args.Array()->get(1));
                     }));
  auto key_params = lepus::Dictionary::Create();
  key_params->SetValue("key", "Enter");
  key_params->SetValue("repeat", false);
  touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", "keydown",
                                          element->impl_id(), key_params);
  EXPECT_EQ(key_count, 1);
  EXPECT_EQ(received_key_detail.Table()->GetValue("key").StdString(), "Enter");

  int wheel_count = 0;
  element->AddEventListener(
      "wheel",
      std::make_shared<event::ClosureEventListener>(
          [&wheel_count](lepus::Value args) {
            ++wheel_count;
            EXPECT_EQ(args.Array()->get(1).Table()->GetValue("deltaY").Number(),
                      12);
          }));
  auto wheel_params = lepus::Dictionary::Create();
  wheel_params->SetValue("deltaY", 12);
  touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", "wheel",
                                          element->impl_id(), wheel_params);
  EXPECT_EQ(wheel_count, 1);
}

TEST_F(TouchEventHandlerTest,
       HandleBubbleInputEventAddsTypeForLegacyDispatcher) {
  auto element =
      tasm_->page_proxy()->element_manager()->CreateFiberElement("view");
  element->MarkAttached();
  auto params = lepus::Dictionary::Create();
  touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", "pointerdown",
                                          element->impl_id(), params);
  EXPECT_EQ(params->GetValue("type").StdString(), "pointerdown");
}

TEST_F(TouchEventHandlerTest, TestHandleTriggerComponentEvent0) {
  touch_event_handler_->HandleTriggerComponentEvent(nullptr, "xxxx",
                                                    lepus::Value());
  EXPECT_EQ(delegate_->DumpDelegate(), "");
}

TEST_F(TouchEventHandlerTest, TestHandleTriggerComponentEvent1) {
  touch_event_handler_->HandleTriggerComponentEvent(
      tasm_.get(), "xxxx", lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(delegate_->DumpDelegate(), "");
}

TEST_F(TouchEventHandlerTest, TestHandleTriggerComponentEvent2) {
  lepus::Value obj(lepus::Dictionary::Create());
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
  tasm_->page_proxy()->element_manager()->SetConfig(tasm_->page_config_);
  DispatchOption option(tasm_->page_proxy());
  component->SetStaticEvent("bindEvent", "xxxx", "onXXXX");
  component->RadonNode::DispatchSelf(option);
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
  EXPECT_EQ(delegate_->DumpDelegate(),
            "SendPageEvent  onXXXX "
            "{\"currentTarget\":{\"dataset\":{},\"id\":\"\"},\"detail\":null,"
            "\"target\":{\"dataset\":{},\"id\":\"\"},\"type\":\"xxxx\"}\n");

  delegate_->ss_.str("");
  delegate_->ss_.clear();
  tasm_->page_config_->SetEnableEventTargetInfoNodeIndex(true);
  touch_event_handler_->HandleTriggerComponentEvent(tasm_.get(), "xxxx", obj);
  EXPECT_EQ(delegate_->DumpDelegate(),
            "SendPageEvent  onXXXX "
            "{\"currentTarget\":{\"dataset\":{},\"id\":\"\",\"nodeIndex\":1},"
            "\"detail\":null,"
            "\"target\":{\"dataset\":{},\"id\":\"\",\"nodeIndex\":1},\"type\":"
            "\"xxxx\"}\n");
}

TEST_F(TouchEventHandlerTest, TestHandleTriggerComponentEvent3) {
  lepus::Value obj(lepus::Dictionary::Create());
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
  lepus::Value obj(lepus::Dictionary::Create());
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
      1, GestureDetectorImpl(
             1, GestureType::PAN,
             {GestureCallback("xxxx", lepus::Value(), lepus::Value())}, {}));
  touch_event_handler_->HandleGestureEvent(tasm_.get(), "xxxx", 10, 1, obj);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
