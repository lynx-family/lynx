// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/events/touch_event_handler.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "build/build_config.h"
#include "core/event/event_dispatcher.h"
#include "core/event/pointer_event.h"
#include "core/event/touch_event.h"
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

TEST_F(TouchEventHandlerTest, RefactoredTouchesBelongToElementManager) {
  auto* manager = tasm_->page_proxy()->element_manager().get();
  auto other_manager = std::make_unique<ElementManager>(
      std::make_unique<MockPaintingContext>(), delegate_.get(),
      manager->GetLynxEnvConfig());
  auto first = manager->CreateFiberElement(base::String("view"));
  auto sibling = manager->CreateFiberElement(base::String("view"));
  auto other = other_manager->CreateFiberElement(base::String("view"));
  first->MarkAttached();
  sibling->MarkAttached();
  other->MarkAttached();
  auto dispatch = [](Element& target, const char* name, int identifier) {
    auto point = lepus::CArray::Create();
    point->emplace_back(identifier);
    for (int i = 0; i < 6; ++i) {
      point->emplace_back(0);
    }
    auto points = lepus::CArray::Create();
    points->emplace_back(std::move(point));
    auto params = lepus::Dictionary::Create();
    params->SetValue(std::to_string(target.impl_id()).c_str(),
                     lepus::Value(std::move(points)));
    auto event =
        fml::MakeRefCounted<event::TouchEvent>(name, lepus::Value(params));
    event::EventDispatcher::DispatchEvent(target, event);
    return event;
  };

  dispatch(*first, EVENT_TOUCH_START, 0);
  auto sibling_start = dispatch(*sibling, EVENT_TOUCH_START, 1);
  EXPECT_EQ(sibling_start->detail().GetProperty("touches").GetLength(), 2u);
  auto other_start = dispatch(*other, EVENT_TOUCH_START, 0);
  EXPECT_EQ(other_start->detail().GetProperty("touches").GetLength(), 1u);

  dispatch(*first, EVENT_TOUCH_CANCEL, 0);
  auto other_move = dispatch(*other, EVENT_TOUCH_MOVE, 0);
  EXPECT_EQ(other_move->detail().GetProperty("touches").GetLength(), 1u);

  // Retained elements and event details can outlive their page owner.
  other_manager.reset();
  EXPECT_EQ(other->GetTouchEventState(), nullptr);
  EXPECT_EQ(other_move->detail().GetProperty("touches").GetLength(), 1u);
}

TEST_F(TouchEventHandlerTest, LongPressListenerCanDestroyPageState) {
  auto manager = std::make_unique<ElementManager>(
      std::make_unique<MockPaintingContext>(), delegate_.get(),
      tasm_->page_proxy()->element_manager()->GetLynxEnvConfig());
  auto target = manager->CreateFiberElement(base::String("view"));
  target->MarkAttached();
  bool called = false;
  target->AddEventListener(EVENT_LONG_PRESS,
                           std::make_shared<event::ClosureEventListener>(
                               [&manager, &called](lepus::Value) {
                                 called = true;
                                 manager.reset();
                               }));
  auto event = fml::MakeRefCounted<event::TouchEvent>(EVENT_LONG_PRESS);

  auto result = event::EventDispatcher::DispatchEvent(*target, event);
  EXPECT_TRUE(called);
  EXPECT_TRUE(result.consumed);
  EXPECT_EQ(manager, nullptr);
  EXPECT_EQ(target->GetTouchEventState(), nullptr);
  EXPECT_EQ(event->detail().GetProperty("touches").GetLength(), 1u);
}

TEST_F(TouchEventHandlerTest, MouseDownResetsPageTouchLongPressState) {
  auto state = tasm_->page_proxy()->element_manager()->GetTouchEventState();
  state->long_press_consumed = true;
  touch_event_handler_->long_press_consumed_ = true;

  touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", "mousedown", 0,
                                          lepus::Dictionary::Create());

  EXPECT_FALSE(state->long_press_consumed);
  EXPECT_FALSE(touch_event_handler_->long_press_consumed_);
}

TEST_F(TouchEventHandlerTest, DestroyedAssemblerIgnoresObservedEvents) {
  const int min_level = base::logging::GetMinLogLevel();
  const int info_level = base::logging::GetInfoLogLevel();
  base::logging::SetMinLogLevel(base::logging::detail::LOG_OBSERVE);
  tasm_->template_loaded_ = true;
  tasm_->Destroy();
  EXPECT_EQ(tasm_->page_proxy()->element_manager(), nullptr);

  tasm_->SendCustomEvent("tap", 1, lepus::Value(), "detail");
  tasm_->SendBubbleEvent("tap", 1, lepus::Dictionary::Create());

  base::logging::SetMinLogLevel(min_level);
  base::logging::detail::g_info_log_level = info_level;
}

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

TEST_F(TouchEventHandlerTest, CurrentTargetPointMatchesBubblingElement) {
  base::String tag("view");
  auto target = tasm_->page_proxy()->element_manager()->CreateFiberElement(tag);
  auto parent = tasm_->page_proxy()->element_manager()->CreateFiberElement(tag);
  EventInfo info(
      target->impl_id(), 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7,
      {{target->impl_id(), 11.f, 12.f}, {parent->impl_id(), 21.f, 22.f}});

  auto target_event = touch_event_handler_->GetTouchEventParam(
      "tap", target.get(), target.get(), info, false);
  auto target_touch = target_event.GetProperty("changedTouches").GetProperty(0);
  EXPECT_FLOAT_EQ(target_touch.GetProperty("currentTargetX").Number(), 11.f);
  EXPECT_FLOAT_EQ(target_touch.GetProperty("currentTargetY").Number(), 12.f);

  auto parent_event = touch_event_handler_->GetTouchEventParam(
      "tap", target.get(), parent.get(), info, false);
  auto parent_touch = parent_event.GetProperty("changedTouches").GetProperty(0);
  EXPECT_FLOAT_EQ(parent_touch.GetProperty("currentTargetX").Number(), 21.f);
  EXPECT_FLOAT_EQ(parent_touch.GetProperty("currentTargetY").Number(), 22.f);
}

TEST_F(TouchEventHandlerTest,
       RefactoredTouchEventMatchesBubblingCurrentTargetPoint) {
  base::String tag("view");
  auto target = tasm_->page_proxy()->element_manager()->CreateFiberElement(tag);
  auto parent = tasm_->page_proxy()->element_manager()->CreateFiberElement(tag);
  auto grandparent =
      tasm_->page_proxy()->element_manager()->CreateFiberElement(tag);
  auto event = fml::MakeRefCounted<event::TouchEvent>(
      "tap", 1.f, 2.f, 5.f, 6.f, 3.f, 4.f, 7,
      event::TouchEventTargetPoints{
          {target->impl_id(), 11.f, 12.f},
          {parent->impl_id(), 21.f, 22.f},
          {grandparent->impl_id(), 31.f, 32.f},
      });
  event->set_target(target->GetWeakTarget());
  event->HandleEventCustomDetail();
  event->set_current_target(parent->GetWeakTarget());
  event->HandleEventBaseDetail();

  auto parent_touch =
      event->detail().GetProperty("changedTouches").GetProperty(0);
  EXPECT_FLOAT_EQ(parent_touch.GetProperty("currentTargetX").Number(), 21.f);
  EXPECT_FLOAT_EQ(parent_touch.GetProperty("currentTargetY").Number(), 22.f);

  // SendGlobalEvent shallow-copies the event detail and freezes nested values.
  auto frozen_changed_touches = event->detail().GetProperty("changedTouches");
  ASSERT_TRUE(frozen_changed_touches.MarkConst());

  event->set_current_target(grandparent->GetWeakTarget());
  event->HandleEventBaseDetail();
  auto grandparent_touch =
      event->detail().GetProperty("changedTouches").GetProperty(0);
  EXPECT_FLOAT_EQ(grandparent_touch.GetProperty("currentTargetX").Number(),
                  31.f);
  EXPECT_FLOAT_EQ(grandparent_touch.GetProperty("currentTargetY").Number(),
                  32.f);
}

TEST_F(TouchEventHandlerTest,
       RefactoredTouchCancelRetainsContactsAndResetsStream) {
  for (bool compatible : {false, true}) {
    SCOPED_TRACE(compatible);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableEventHandleRefactor(true);
    config->SetEnableMultiTouchParamsCompatible(compatible);
    tasm_->page_config_ = config;
    auto* manager = tasm_->page_proxy()->element_manager().get();
    manager->SetConfig(config);
    auto target = manager->CreateFiberElement("view");
    target->MarkAttached();
    auto other = manager->CreateFiberElement("view");
    other->MarkAttached();
    std::vector<lepus::Value> received;
    const auto dispatch = [&](const std::string& name, int id) {
      auto params = lepus::Dictionary::Create();
      const auto ids = name == EVENT_TOUCH_CANCEL ? std::vector<int>{7, 11}
                                                  : std::vector<int>{id};
      for (int identifier : ids) {
        auto point = lepus::CArray::Create();
        for (int value : {identifier, 10, 20, 10, 20, 1, 2}) {
          point->emplace_back(value);
        }
        auto points = lepus::CArray::Create();
        points->emplace_back(std::move(point));
        params->SetValue(std::to_string(identifier == 11 ? other->impl_id()
                                                         : target->impl_id()),
                         std::move(points));
      }
      touch_event_handler_->HandleTouchEvent(tasm_.get(), "", name,
                                             EventInfo(lepus::Value(params)));
    };
    for (auto* element : {target.get(), other.get()}) {
      for (const char* name : {EVENT_TOUCH_START, EVENT_TOUCH_MOVE,
                               EVENT_TOUCH_END, EVENT_TOUCH_CANCEL}) {
        element->AddEventListener(
            name, std::make_shared<event::ClosureEventListener>(
                      [&received](lepus::Value args) {
                        received.push_back(args.Array()->get(1));
                      }));
      }
    }
    dispatch(EVENT_TOUCH_START, 7);
    dispatch(EVENT_TOUCH_START, 11);
    dispatch(EVENT_TOUCH_CANCEL, 7);
    EXPECT_EQ(manager->GetTouchEventState()->current_touches.GetLength(), 0u);
    dispatch(EVENT_TOUCH_START, 9);
    dispatch(EVENT_TOUCH_MOVE, 9);
    dispatch(EVENT_TOUCH_END, 9);
    ASSERT_EQ(received.size(), 7u);
    EXPECT_EQ(received[0].GetProperty("touches").GetLength(), 1u);
    EXPECT_EQ(received[1].GetProperty("touches").GetLength(), 2u);
    for (size_t index : {2u, 3u}) {
      const auto changed = received[index].GetProperty("changedTouches");
      ASSERT_EQ(changed.GetLength(), 2u);
      EXPECT_EQ(changed.GetProperty(0).GetProperty("identifier").Number(), 7);
      EXPECT_EQ(changed.GetProperty(1).GetProperty("identifier").Number(), 11);
      EXPECT_EQ(received[index].GetProperty("touches").GetLength(),
                compatible ? 2u : 0u);
    }
    for (size_t index : {4u, 5u}) {
      const auto touches = received[index].GetProperty("touches");
      ASSERT_EQ(touches.GetLength(), 1u);
      EXPECT_EQ(touches.GetProperty(0).GetProperty("identifier").Number(), 9);
    }
    EXPECT_EQ(received[6].GetProperty("touches").GetLength(),
              compatible ? 1u : 0u);
  }
}

TEST_F(TouchEventHandlerTest, TouchCancelListenerCanDestroyPageState) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableEventHandleRefactor(true);
  tasm_->page_config_ = config;
  auto* manager = tasm_->page_proxy()->element_manager().get();
  manager->SetConfig(config);
  auto target = manager->CreateFiberElement("view");
  target->MarkAttached();
  auto other = manager->CreateFiberElement("view");
  other->MarkAttached();
  int call_count = 0;
  for (auto* element : {target.get(), other.get()}) {
    element->AddEventListener(EVENT_TOUCH_CANCEL,
                              std::make_shared<event::ClosureEventListener>(
                                  [this, &call_count](lepus::Value) {
                                    ++call_count;
                                    tasm_->Destroy();
                                  }));
  }
  auto params = lepus::Dictionary::Create();
  params->SetValue(std::to_string(target->impl_id()), lepus::CArray::Create());
  params->SetValue(std::to_string(other->impl_id()), lepus::CArray::Create());
  touch_event_handler_->HandleTouchEvent(tasm_.get(), "", EVENT_TOUCH_CANCEL,
                                         EventInfo(lepus::Value(params)));
  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(tasm_->page_proxy()->element_manager(), nullptr);
}

TEST_F(TouchEventHandlerTest, ScalarTouchCancelClearsAfterListeners) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableEventHandleRefactor(true);
  tasm_->page_config_ = config;
  auto* manager = tasm_->page_proxy()->element_manager().get();
  manager->SetConfig(config);
  auto target = manager->CreateFiberElement("view");
  target->MarkAttached();
  manager->GetTouchEventState()->current_touches.Array()->emplace_back(7);
  lepus::Value received;
  target->AddEventListener(
      EVENT_TOUCH_CANCEL,
      std::make_shared<event::ClosureEventListener>([&](lepus::Value args) {
        EXPECT_EQ(manager->GetTouchEventState()->current_touches.GetLength(),
                  1u);
        received = args.Array()->get(1);
      }));
  touch_event_handler_->HandleTouchEvent(
      tasm_.get(), "", EVENT_TOUCH_CANCEL,
      EventInfo(target->impl_id(), 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7,
                {{target->impl_id(), 11.f, 12.f}}));
  EXPECT_EQ(manager->GetTouchEventState()->current_touches.GetLength(), 0u);
  const auto changed = received.GetProperty("changedTouches");
  ASSERT_EQ(changed.GetLength(), 1u);
  const auto touch = changed.GetProperty(0);
  for (const auto& coordinate : {std::pair<const char*, float>{"x", 1.f},
                                 {"y", 2.f},
                                 {"clientX", 3.f},
                                 {"clientY", 4.f},
                                 {"pageX", 5.f},
                                 {"pageY", 6.f},
                                 {"currentTargetX", 11.f},
                                 {"currentTargetY", 12.f}}) {
    EXPECT_FLOAT_EQ(touch.GetProperty(coordinate.first).Number(),
                    coordinate.second);
  }
}

TEST_F(TouchEventHandlerTest, ScalarTouchCancelListenerCanDestroyPageState) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableEventHandleRefactor(true);
  tasm_->page_config_ = config;
  auto* manager = tasm_->page_proxy()->element_manager().get();
  manager->SetConfig(config);
  auto target = manager->CreateFiberElement("view");
  target->MarkAttached();
  int call_count = 0;
  target->AddEventListener(EVENT_TOUCH_CANCEL,
                           std::make_shared<event::ClosureEventListener>(
                               [this, &call_count](lepus::Value) {
                                 ++call_count;
                                 tasm_->Destroy();
                               }));
  touch_event_handler_->HandleTouchEvent(
      tasm_.get(), "", EVENT_TOUCH_CANCEL,
      EventInfo(target->impl_id(), 1.f, 2.f, 3.f, 4.f, 5.f, 6.f));
  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(tasm_->page_proxy()->element_manager(), nullptr);
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

#if defined(OS_WIN) || defined(OS_MAC) || defined(OS_LINUX)
TEST_F(TouchEventHandlerTest, HandleAllPointerEventsWithRefactoredDispatcher) {
  auto page_config = std::make_shared<PageConfig>();
  page_config->SetEnableEventHandleRefactor(true);
  tasm_->page_config_ = page_config;
  auto* manager = tasm_->page_proxy()->element_manager().get();
  manager->SetConfig(page_config);
  auto parent = manager->CreateFiberElement("view");
  auto target = manager->CreateFiberElement("view");
  auto related_target = manager->CreateFiberElement("view");
  parent->MarkAttached();
  target->MarkAttached();
  related_target->MarkAttached();
  related_target->SetIdSelector("related");
  parent->InsertNode(target);
  constexpr const char* kPointerEvents[] = {
      "pointerdown",  "pointermove",  "pointerup",   "pointercancel",
      "pointerenter", "pointerleave", "pointerover", "pointerout",
  };
  std::unordered_map<std::string, int> call_counts;
  std::unordered_map<std::string, int> capture_counts;
  std::unordered_map<std::string, int> bubble_counts;
  std::unordered_map<std::string, lepus::Value> received_details;
  for (const auto* name : kPointerEvents) {
    parent->AddEventListener(
        name,
        std::make_shared<event::ClosureEventListener>(
            [name, &capture_counts](lepus::Value) { ++capture_counts[name]; },
            event::EventListener::Options(true)));
    parent->AddEventListener(
        name,
        std::make_shared<event::ClosureEventListener>(
            [name, &bubble_counts](lepus::Value) { ++bubble_counts[name]; }));
    target->AddEventListener(
        name, std::make_shared<event::ClosureEventListener>(
                  [name, &call_counts, &received_details](lepus::Value args) {
                    ++call_counts[name];
                    received_details.insert_or_assign(
                        name, lepus::Value::Clone(args.Array()->get(1)));
                  }));
  }
  for (const auto* name : kPointerEvents) {
    SCOPED_TRACE(name);
    auto params = lepus::Dictionary::Create();
    params->SetValue("pointerId", 3);
    params->SetValue("pointerType", "mouse");
    params->SetValue("timestamp", 1234);
    params->SetValue(event::kPointerRelatedTargetSign,
                     related_target->impl_id());
    touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", name,
                                            target->impl_id(), params);
    ASSERT_EQ(call_counts[name], 1);
    EXPECT_EQ(capture_counts[name], 1);
    EXPECT_EQ(bubble_counts[name], std::string(name) == "pointerenter" ||
                                           std::string(name) == "pointerleave"
                                       ? 0
                                       : 1);
    const auto& detail = received_details.at(name);
    ASSERT_TRUE(detail.IsTable());
    EXPECT_EQ(detail.Table()->GetValue("type").StdString(), name);
    EXPECT_EQ(detail.Table()->GetValue("pointerId").Number(), 3);
    EXPECT_EQ(detail.Table()->GetValue("pointerType").StdString(), "mouse");
    EXPECT_EQ(detail.Table()->GetValue("timestamp").Number(), 1234);
    EXPECT_EQ(detail.Table()->GetValue("width").Number(), 1);
    EXPECT_EQ(detail.Table()->GetValue("height").Number(), 1);
    EXPECT_EQ(detail.Table()->GetValue("pressure").Number(), 0);
    EXPECT_FALSE(detail.Table()->Contains(event::kPointerRelatedTargetSign));
    const auto& related = detail.Table()->GetValue("relatedTarget");
    ASSERT_TRUE(related.IsTable());
    EXPECT_EQ(related.Table()->GetValue("id").StdString(), "related");
  }
}

TEST_F(TouchEventHandlerTest, PointerEventsRequireRefactoredDispatcher) {
  auto page_config = std::make_shared<PageConfig>();
  page_config->SetEnableEventHandleRefactor(false);
  tasm_->page_config_ = page_config;
  tasm_->page_proxy()->element_manager()->SetConfig(page_config);
  ASSERT_FALSE(tasm_->EnableEventHandleRefactor());
  auto element =
      tasm_->page_proxy()->element_manager()->CreateFiberElement("view");
  element->MarkAttached();
  constexpr const char* kPointerEvents[] = {
      "pointerdown",  "pointermove",  "pointerup",   "pointercancel",
      "pointerenter", "pointerleave", "pointerover", "pointerout",
  };
  int new_dispatch_count = 0;
  for (const auto* name : kPointerEvents) {
    SCOPED_TRACE(name);
    element->SetJSEventHandler(name, "bindEvent", "legacyPointer");
    element->AddEventListener(
        name,
        std::make_shared<event::ClosureEventListener>(
            [&new_dispatch_count](lepus::Value) { ++new_dispatch_count; }));
    auto params = lepus::Dictionary::Create();
    params->SetValue(event::kPointerRelatedTargetSign,
                     event::kInvalidPointerRelatedTargetSign);
    params->SetValue("timestamp", 1234);
    touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", name,
                                            element->impl_id(), params);
    EXPECT_EQ(delegate_->DumpDelegate(), "");
    EXPECT_EQ(new_dispatch_count, 0);
    EXPECT_FALSE(params->Contains("type"));
    EXPECT_FALSE(params->Contains("width"));
    EXPECT_EQ(params->GetValue("timestamp").Number(), 1234);
    EXPECT_TRUE(params->Contains(event::kPointerRelatedTargetSign));
  }
  element->SetJSEventHandler("mousedown", "bindEvent", "legacyMouse");
  touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", "mousedown",
                                          element->impl_id(),
                                          lepus::Dictionary::Create());
  EXPECT_NE(delegate_->DumpDelegate().find("legacyMouse"), std::string::npos);
}

TEST_F(TouchEventHandlerTest, MouseBoundariesKeepLegacyPropagation) {
  auto* manager = tasm_->page_proxy()->element_manager().get();
  auto outer = manager->CreateFiberElement("view");
  auto component = manager->CreateFiberComponent("component-id", 1, "entry",
                                                 "Component", "/component");
  auto bubble_parent = manager->CreateFiberElement("view");
  auto capture_parent = manager->CreateFiberElement("view");
  auto target = manager->CreateFiberElement("view");
  outer->MarkAttached();
  component->MarkAttached();
  bubble_parent->MarkAttached();
  capture_parent->MarkAttached();
  target->MarkAttached();
  outer->InsertNode(component);
  component->InsertNode(bubble_parent);
  bubble_parent->InsertNode(capture_parent);
  capture_parent->InsertNode(target);
  for (auto* element :
       {bubble_parent.get(), capture_parent.get(), target.get()}) {
    element->SetParentComponentUniqueIdForFiber(component->impl_id());
  }
  for (bool enabled : {false, true}) {
    auto page_config = std::make_shared<PageConfig>();
    page_config->SetEnableEventHandleRefactor(enabled);
    tasm_->page_config_ = page_config;
    manager->SetConfig(page_config);
    for (const auto* name : {"mouseenter", "mouseleave"}) {
      SCOPED_TRACE(name);
      capture_parent->SetJSEventHandler(name, "capture-bind", "parentCapture");
      bubble_parent->SetJSEventHandler(name, "bindEvent", "bubble");
      outer->SetJSEventHandler(name, "capture-bind", "outerCapture");
      target->SetJSEventHandler(name, "bindEvent", "targetMouse");
      delegate_->ss_.str("");
      touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", name,
                                              target->impl_id(),
                                              lepus::Dictionary::Create());
      const auto& output = delegate_->DumpDelegate();
      EXPECT_EQ(output.find("parentCapture"), std::string::npos);
      EXPECT_EQ(output.find("bubble"), std::string::npos);
      EXPECT_EQ(output.find("outerCapture"), std::string::npos);
      EXPECT_NE(output.find("targetMouse"), std::string::npos);
    }
  }
}

TEST_F(TouchEventHandlerTest, NonComposedPointerEventStopsAtComponentBoundary) {
  auto page_config = std::make_shared<PageConfig>();
  page_config->SetEnableEventHandleRefactor(true);
  tasm_->page_config_ = page_config;
  auto* manager = tasm_->page_proxy()->element_manager().get();
  manager->SetConfig(page_config);
  auto outer = manager->CreateFiberElement("view");
  auto component = manager->CreateFiberComponent("component-id", 1, "entry",
                                                 "Component", "/component");
  auto target = manager->CreateFiberElement("view");
  outer->MarkAttached();
  component->MarkAttached();
  target->MarkAttached();
  outer->InsertNode(component);
  target->SetParentComponentUniqueIdForFiber(component->impl_id());
  component->InsertNode(target);
  int enter_count = 0;
  int over_count = 0;
  outer->AddEventListener("pointerenter",
                          std::make_shared<event::ClosureEventListener>(
                              [&enter_count](lepus::Value) { ++enter_count; },
                              event::EventListener::Options(true)));
  outer->AddEventListener("pointerover",
                          std::make_shared<event::ClosureEventListener>(
                              [&over_count](lepus::Value) { ++over_count; },
                              event::EventListener::Options(true)));

  touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", "pointerenter",
                                          target->impl_id(),
                                          lepus::Dictionary::Create());
  touch_event_handler_->HandleBubbleEvent(tasm_.get(), "", "pointerover",
                                          target->impl_id(),
                                          lepus::Dictionary::Create());

  EXPECT_EQ(enter_count, 0);
  EXPECT_EQ(over_count, 1);
}
#endif

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
