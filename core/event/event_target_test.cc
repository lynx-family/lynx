// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/event_target_test.h"

#include "core/event/event_listener_map.h"
#include "core/event/event_listener_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace event {
namespace test {

EventTarget* MockEventTarget::GetParentTarget() { return nullptr; }

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

  Event mock_event("test", Event::EventType::kTouchEvent, Event::Bubbles::kNo,
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

  Event mock_event("test", Event::EventType::kTouchEvent, Event::Bubbles::kNo,
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

  Event mock_event("test", Event::EventType::kTouchEvent, Event::Bubbles::kNo,
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

  Event mock_event("test", Event::EventType::kTouchEvent, Event::Bubbles::kNo,
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

}  // namespace test
}  // namespace event
}  // namespace lynx
