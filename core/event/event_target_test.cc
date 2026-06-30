// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/event_target_test.h"

#include "core/event/custom_event.h"
#include "core/event/event_dispatcher.h"
#include "core/event/event_listener_map.h"
#include "core/event/event_listener_test.h"
#include "core/event/touch_event.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace event {
namespace test {

EventTarget* MockEventTarget::GetParentTarget() { return parent_; }

namespace {

lepus::Value CreateEventTargetInfo(const std::string& id,
                                   const std::string& dataset_value) {
  auto dict = lepus::Dictionary::Create();
  auto dataset = lepus::Dictionary::Create();
  dataset->SetValue("scene", dataset_value);
  dict->SetValue("id", id);
  dict->SetValue("dataset", std::move(dataset));
  return lepus::Value(std::move(dict));
}

}  // namespace

TEST_F(EventTargetTest, TestEventTargetTest0) {
  auto map = mock_target_->GetEventListenerMap();

  // init event listener map
  EXPECT_TRUE(map->IsEmpty());
  EXPECT_FALSE(map->Contains("test"));
  auto* vec_ptr = map->Find("test");
  EXPECT_EQ(vec_ptr, nullptr);
  EXPECT_FALSE(mock_target_->RemoveEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));

  auto mock_event = fml::MakeRefCounted<Event>(
      "test", Event::EventType::kNone, Event::Capture::kNo, Event::Bubbles::kNo,
      Event::Cancelable::kYes, Event::ComposedMode::kComposed,
      Event::PhaseType::kNone);

  // add {"test": "1"}
  mock_target_->AddEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1"));
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 1);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            1);

  // add {"test": "2"}
  mock_target_->AddEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "2"));
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 2);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "2");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            1);

  // add {"test": "3"}
  mock_target_->AddEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "3"));
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 3);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "2");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetContent(),
            "3");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            3);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            1);

  EXPECT_FALSE(mock_target_->RemoveEventListener(
      "test1", std::make_unique<MockEventListener>(
                   EventListener::Type::kJSClosureEventListener, "1")));

  // remove {"test": "1"}
  EXPECT_TRUE(mock_target_->RemoveEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 2);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "2");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "3");

  // clear
  map->Clear();
  EXPECT_TRUE(map->IsEmpty());
  EXPECT_FALSE(map->Contains("test"));
  vec_ptr = map->Find("test");
  EXPECT_EQ(vec_ptr, nullptr);
  EXPECT_FALSE(map->Remove(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));
}

TEST_F(EventTargetTest, TestEventTargetEraseBefore) {
  auto map = mock_target_->GetEventListenerMap();

  // init event listener map
  EXPECT_TRUE(map->IsEmpty());
  EXPECT_FALSE(map->Contains("test"));
  auto* vec_ptr = map->Find("test");
  EXPECT_EQ(vec_ptr, nullptr);
  EXPECT_FALSE(mock_target_->RemoveEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));

  auto mock_event = fml::MakeRefCounted<Event>(
      "test", Event::EventType::kNone, Event::Capture::kNo, Event::Bubbles::kNo,
      Event::Cancelable::kYes, Event::ComposedMode::kComposed,
      Event::PhaseType::kNone);

  // add {"test": "1"}
  mock_target_->AddEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1"));
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 1);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            1);

  // add {"test": "2"}
  mock_target_->AddEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "2", "test",
                  "1", mock_target_.get()));
  mock_target_->AddEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "3", "test",
                  "1", mock_target_.get()));
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 3);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "2");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetContent(),
            "3");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "2");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "3");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            1);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            1);
}

TEST_F(EventTargetTest, TestEventTargetEraseCurrent) {
  auto map = mock_target_->GetEventListenerMap();

  // init event listener map
  EXPECT_TRUE(map->IsEmpty());
  EXPECT_FALSE(map->Contains("test"));
  auto* vec_ptr = map->Find("test");
  EXPECT_EQ(vec_ptr, nullptr);
  EXPECT_FALSE(mock_target_->RemoveEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));

  auto mock_event = fml::MakeRefCounted<Event>(
      "test", Event::EventType::kNone, Event::Capture::kNo, Event::Bubbles::kNo,
      Event::Cancelable::kYes, Event::ComposedMode::kComposed,
      Event::PhaseType::kNone);

  // add {"test": "1"}
  mock_target_->AddEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1"));
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 1);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            1);

  // add {"test": "2"}
  auto event_l_2 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "2", "test", "2",
      mock_target_.get());
  mock_target_->AddEventListener("test", event_l_2);
  auto event_l_3 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "3", "test", "3",
      mock_target_.get());
  mock_target_->AddEventListener("test", event_l_3);
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 3);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "2");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetContent(),
            "3");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);

  EXPECT_EQ(static_cast<MockEventListener*>(event_l_2.get())->GetCount(), 1);
  EXPECT_EQ(static_cast<MockEventListener*>(event_l_3.get())->GetCount(), 1);
}

TEST_F(EventTargetTest, TestEventTargetEraseAfter) {
  auto map = mock_target_->GetEventListenerMap();

  // init event listener map
  EXPECT_TRUE(map->IsEmpty());
  EXPECT_FALSE(map->Contains("test"));
  auto* vec_ptr = map->Find("test");
  EXPECT_EQ(vec_ptr, nullptr);
  EXPECT_FALSE(mock_target_->RemoveEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));

  auto mock_event = fml::MakeRefCounted<Event>(
      "test", Event::EventType::kNone, Event::Capture::kNo, Event::Bubbles::kNo,
      Event::Cancelable::kYes, Event::ComposedMode::kComposed,
      Event::PhaseType::kNone);

  // add {"test": "1"}
  mock_target_->AddEventListener(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1"));
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 1);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            1);

  // add {"test": "2"}
  auto event_l_2 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "2", "test", "3",
      mock_target_.get());
  mock_target_->AddEventListener("test", event_l_2);
  auto event_l_3 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "3", "test", "0",
      mock_target_.get());
  mock_target_->AddEventListener("test", event_l_3);
  EXPECT_FALSE(map->IsEmpty());
  vec_ptr = map->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 3);
  EXPECT_TRUE(map->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "2");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetContent(),
            "3");
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);

  EXPECT_EQ(static_cast<MockEventListener*>(event_l_2.get())->GetCount(), 1);
  EXPECT_EQ(static_cast<MockEventListener*>(event_l_3.get())->GetCount(), 0);
}

TEST_F(EventTargetTest, TestEventTargetDispatchCaptureBubbleEvent) {
  auto map = mock_target_->GetEventListenerMap();
  auto mock_event = fml::MakeRefCounted<TouchEvent>("tap");

  // add {"tap": "1"} with capture flag
  auto event_options_1 = event::EventListener::Options(
      true /* capture */, false, false, false, false, false);
  auto event_l_1 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "1", "tap", "0",
      mock_target_.get(), event_options_1);
  mock_target_->AddEventListener("tap", event_l_1);

  // add {"tap": "2"} with bubble flag
  auto event_options_2 = event::EventListener::Options(
      false /* bubble */, false, false, false, false, false);
  auto event_l_2 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "2", "tap", "0",
      mock_target_.get(), event_options_2);
  mock_target_->AddEventListener("tap", event_l_2);

  // add {"tap": "3"} with global flag
  auto event_options_3 = event::EventListener::Options(
      false, false, false, false, false, true /* global */);
  auto event_l_3 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "3", "tap", "0",
      mock_target_.get(), event_options_3);
  mock_target_->AddEventListener("tap", event_l_3);

  EXPECT_FALSE(map->IsEmpty());
  auto vec_ptr = map->Find("tap");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 3);

  mock_event->set_event_phase(Event::PhaseType::kCapturingPhase);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            1);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            0);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            0);

  mock_event->set_event_phase(Event::PhaseType::kAtTarget);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            1);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            0);

  mock_event->set_event_phase(Event::PhaseType::kBubblingPhase);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            0);

  mock_event->set_event_phase(Event::PhaseType::kGlobal);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            1);
}

TEST_F(EventTargetTest, TestEventTargetDispatchNoCaptureBubbleEvent) {
  auto map = mock_target_->GetEventListenerMap();
  auto mock_event =
      fml::MakeRefCounted<CustomEvent>("custom", lepus::Value(), "detail");

  // add {"custom": "1"} with capture flag
  auto event_options_1 = event::EventListener::Options(
      true /* capture */, false, false, false, false, false);
  auto event_l_1 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "1", "custom", "0",
      mock_target_.get(), event_options_1);
  mock_target_->AddEventListener("custom", event_l_1);

  // add {"custom": "2"} with bubble flag
  auto event_options_2 = event::EventListener::Options(
      false /* bubble */, false, false, false, false, false);
  auto event_l_2 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "2", "custom", "0",
      mock_target_.get(), event_options_2);
  mock_target_->AddEventListener("custom", event_l_2);

  // add {"custom": "3"} with global flag
  auto event_options_3 = event::EventListener::Options(
      false, false, false, false, false, true /* global */);
  auto event_l_3 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "3", "custom", "0",
      mock_target_.get(), event_options_3);
  mock_target_->AddEventListener("custom", event_l_3);

  EXPECT_FALSE(map->IsEmpty());
  auto vec_ptr = map->Find("custom");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 3);

  mock_event->set_capture(true);
  mock_event->set_bubbles(true);
  mock_event->set_event_phase(Event::PhaseType::kCapturingPhase);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            1);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            0);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            0);

  mock_event->set_event_phase(Event::PhaseType::kAtTarget);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            1);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            0);

  mock_event->set_event_phase(Event::PhaseType::kBubblingPhase);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            0);

  mock_event->set_event_phase(Event::PhaseType::kGlobal);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            1);

  mock_event->set_capture(false);
  mock_event->set_event_phase(Event::PhaseType::kAtTarget);
  mock_target_->DispatchEvent(mock_event);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetCount(),
            2);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetCount(),
            3);
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetCount(),
            1);
}

TEST_F(EventTargetTest,
       FrontendCustomEventBubbleCompatibleDispatchesFirstAncestorOnly) {
  auto parent = std::make_unique<MockEventTarget>();
  auto grandparent = std::make_unique<MockEventTarget>();
  mock_target_->SetParentTarget(parent.get());
  parent->SetParentTarget(grandparent.get());

  auto target_capture_options = EventListener::Options(
      true /* capture */, false, false, false, false, false);
  auto target_capture_listener = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "target_capture", "custom",
      "0", mock_target_.get(), target_capture_options);
  mock_target_->AddEventListener("custom", target_capture_listener);

  auto parent_listener = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "parent");
  parent->AddEventListener("custom", parent_listener);
  auto grandparent_listener = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "grandparent");
  grandparent->AddEventListener("custom", grandparent_listener);

  auto event = fml::MakeRefCounted<CustomEvent>(
      "custom", lepus::Value(), "detail", 0, Event::Capture::kYes,
      Event::Bubbles::kNo, Event::Cancelable::kYes,
      Event::ComposedMode::kComposed, Event::PhaseType::kNone);
  event->set_from_frontend(true);
  event->set_enable_frontend_custom_event_bubble_compatible(true);

  auto result = EventDispatcher::DispatchEvent(*mock_target_, event);

  EXPECT_EQ(result.cancel_type, EventCancelType::kNotCanceled);
  EXPECT_TRUE(result.consumed);
  EXPECT_EQ(target_capture_listener->GetCount(), 1);
  EXPECT_EQ(parent_listener->GetCount(), 1);
  EXPECT_EQ(grandparent_listener->GetCount(), 0);
}

TEST_F(EventTargetTest,
       FrontendCustomEventBubbleCompatibleSkipsTargetWithBubble) {
  auto parent = std::make_unique<MockEventTarget>();
  mock_target_->SetParentTarget(parent.get());

  auto target_listener = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "target");
  mock_target_->AddEventListener("custom", target_listener);
  auto parent_listener = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "parent");
  parent->AddEventListener("custom", parent_listener);

  auto event = fml::MakeRefCounted<CustomEvent>(
      "custom", lepus::Value(), "detail", 0, Event::Capture::kNo,
      Event::Bubbles::kNo, Event::Cancelable::kYes,
      Event::ComposedMode::kComposed, Event::PhaseType::kNone);
  event->set_from_frontend(true);
  event->set_enable_frontend_custom_event_bubble_compatible(true);

  auto result = EventDispatcher::DispatchEvent(*mock_target_, event);

  EXPECT_EQ(result.cancel_type, EventCancelType::kNotCanceled);
  EXPECT_TRUE(result.consumed);
  EXPECT_EQ(target_listener->GetCount(), 1);
  EXPECT_EQ(parent_listener->GetCount(), 0);
}

TEST_F(EventTargetTest, FrontendCustomEventBubbleCompatibleDisabled) {
  auto parent = std::make_unique<MockEventTarget>();
  mock_target_->SetParentTarget(parent.get());

  auto parent_listener = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "parent");
  parent->AddEventListener("custom", parent_listener);

  auto event = fml::MakeRefCounted<CustomEvent>(
      "custom", lepus::Value(), "detail", 0, Event::Capture::kNo,
      Event::Bubbles::kNo, Event::Cancelable::kYes,
      Event::ComposedMode::kComposed, Event::PhaseType::kNone);
  event->set_from_frontend(true);
  event->set_enable_frontend_custom_event_bubble_compatible(false);

  auto result = EventDispatcher::DispatchEvent(*mock_target_, event);

  EXPECT_EQ(result.cancel_type, EventCancelType::kNotCanceled);
  EXPECT_FALSE(result.consumed);
  EXPECT_EQ(parent_listener->GetCount(), 0);
}

TEST_F(EventTargetTest, CustomEventParamsDetailOnlyForNativeEvent) {
  auto params = lepus::Dictionary::Create();
  params->SetValue("foo", "bar");

  auto native_event = fml::MakeRefCounted<CustomEvent>(
      "custom", lepus::Value(params), "params");
  native_event->HandleEventCustomDetail();
  ASSERT_TRUE(native_event->detail().IsTable());
  EXPECT_TRUE(native_event->detail().Table()->GetValue("detail").IsObject());

  auto frontend_event = fml::MakeRefCounted<CustomEvent>(
      "custom", lepus::Value(params), "params");
  frontend_event->set_from_frontend(true);
  frontend_event->HandleEventCustomDetail();
  ASSERT_TRUE(frontend_event->detail().IsTable());
  EXPECT_TRUE(frontend_event->detail().Table()->GetValue("detail").IsNil());
}

TEST_F(EventTargetTest, LegacyNativeCustomEventParam) {
  auto params = lepus::Dictionary::Create();
  params->SetValue("foo", "bar");
  mock_target_->SetEventTargetInfo(CreateEventTargetInfo("target", "dataset"));

  auto event = fml::MakeRefCounted<CustomEvent>("custom", lepus::Value(params),
                                                "params");
  event->set_enable_legacy_native_event_param(true);
  event->set_target(mock_target_->GetWeakTarget());
  event->HandleEventCustomDetail();
  event->set_current_target(mock_target_->GetWeakTarget());
  event->HandleEventBaseDetail();

  ASSERT_TRUE(event->detail().IsTable());
  auto detail = event->detail().Table();
  EXPECT_EQ(detail->GetValue("id").StdString(), "target");
  ASSERT_TRUE(detail->GetValue("dataset").IsTable());
  EXPECT_EQ(detail->GetValue("dataset").Table()->GetValue("scene").StdString(),
            "dataset");
  ASSERT_TRUE(detail->GetValue("target").IsTable());
  ASSERT_TRUE(detail->GetValue("currentTarget").IsTable());
  EXPECT_EQ(detail->GetValue("target")
                .Table()
                ->GetValue("params")
                .Table()
                ->GetValue("foo")
                .StdString(),
            "bar");
  EXPECT_EQ(detail->GetValue("currentTarget")
                .Table()
                ->GetValue("params")
                .Table()
                ->GetValue("foo")
                .StdString(),
            "bar");
}

TEST_F(EventTargetTest, LegacyFrontendCustomEventObjectParam) {
  auto params = lepus::Dictionary::Create();
  params->SetValue("type", "legacy");
  params->SetValue("timestamp", 123);
  params->SetValue("detail", "legacy_detail");
  params->SetValue("foo", "bar");

  auto event = fml::MakeRefCounted<CustomEvent>(
      "custom", lepus::Value(params), "detail", 0, Event::Capture::kNo,
      Event::Bubbles::kNo, Event::Cancelable::kYes,
      Event::ComposedMode::kScoped, Event::PhaseType::kNone, true);
  event->set_from_frontend(true);
  event->HandleEventCustomDetail();
  event->HandleEventBaseDetail();

  ASSERT_TRUE(event->detail().IsTable());
  EXPECT_EQ(event->detail().Table()->GetValue("type").StdString(), "legacy");
  EXPECT_EQ(static_cast<int32_t>(
                event->detail().Table()->GetValue("timestamp").Number()),
            123);
  EXPECT_EQ(event->detail().Table()->GetValue("detail").StdString(),
            "legacy_detail");
  EXPECT_EQ(event->detail().Table()->GetValue("foo").StdString(), "bar");
}

TEST_F(EventTargetTest, LegacyFrontendCustomEventNonObjectParam) {
  auto event = fml::MakeRefCounted<CustomEvent>(
      "custom", lepus::Value("legacy"), "detail", 0, Event::Capture::kNo,
      Event::Bubbles::kNo, Event::Cancelable::kYes,
      Event::ComposedMode::kScoped, Event::PhaseType::kNone, true);
  event->set_from_frontend(true);
  event->HandleEventCustomDetail();
  event->HandleEventBaseDetail();

  ASSERT_TRUE(event->detail().IsString());
  EXPECT_EQ(event->detail().StdString(), "legacy");
}

}  // namespace test
}  // namespace event
}  // namespace lynx
