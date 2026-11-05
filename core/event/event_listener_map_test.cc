// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/event_listener_map_test.h"

#include "core/event/event_listener_test.h"

namespace lynx {
namespace event {
namespace test {

TEST_F(EventListenerMapTest, TestEventListenerMapTest0) {
  // init event listener map
  EXPECT_TRUE(map_->IsEmpty());
  EXPECT_FALSE(map_->Contains("test"));
  auto* vec_ptr = map_->Find("test");
  EXPECT_EQ(vec_ptr, nullptr);
  EXPECT_FALSE(map_->Remove(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));

  // add {"test": "1"}
  map_->Add("test", std::make_unique<MockEventListener>(
                        EventListener::Type::kJSClosureEventListener, "1"));
  EXPECT_FALSE(map_->IsEmpty());
  vec_ptr = map_->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 1);
  EXPECT_TRUE(map_->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");

  // add {"test": "2"}
  map_->Add("test", std::make_unique<MockEventListener>(
                        EventListener::Type::kJSClosureEventListener, "2"));
  EXPECT_FALSE(map_->IsEmpty());
  vec_ptr = map_->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 2);
  EXPECT_TRUE(map_->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "2");

  // add {"test": "3"}
  map_->Add("test", std::make_unique<MockEventListener>(
                        EventListener::Type::kJSClosureEventListener, "3"));
  EXPECT_FALSE(map_->IsEmpty());
  vec_ptr = map_->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 3);
  EXPECT_TRUE(map_->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "1");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "2");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[2].get())->GetContent(),
            "3");

  EXPECT_FALSE(map_->Remove(
      "test1", std::make_unique<MockEventListener>(
                   EventListener::Type::kJSClosureEventListener, "1")));

  // remove {"test": "1"}
  EXPECT_TRUE(map_->Remove(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));
  EXPECT_FALSE(map_->IsEmpty());
  vec_ptr = map_->Find("test");
  EXPECT_EQ(static_cast<int32_t>(vec_ptr->size()), 2);
  EXPECT_TRUE(map_->Contains("test"));
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[0].get())->GetContent(),
            "2");
  EXPECT_EQ(static_cast<MockEventListener*>((*vec_ptr)[1].get())->GetContent(),
            "3");

  // remove all
  map_->RemoveAll();
  EXPECT_TRUE(map_->IsEmpty());
  EXPECT_FALSE(map_->Contains("test"));
  vec_ptr = map_->Find("test");
  EXPECT_EQ(vec_ptr, nullptr);
  EXPECT_FALSE(map_->Remove(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));
}

TEST_F(EventListenerMapTest, RemoveEventTypeMarksListenersAsRemoved) {
  auto listener_1 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "1");
  auto listener_2 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "2");
  map_->Add("test", listener_1);
  map_->Add("test", listener_2);

  EXPECT_TRUE(map_->Remove("test"));

  EXPECT_TRUE(listener_1->removed());
  EXPECT_TRUE(listener_2->removed());
  EXPECT_TRUE(map_->IsEmpty());
  EXPECT_FALSE(map_->Contains("test"));
  EXPECT_EQ(map_->Find("test"), nullptr);
  EXPECT_FALSE(map_->Remove("test"));
}

TEST_F(EventListenerMapTest, ContainsRequiresNonemptyListenerList) {
  map_->Add("test", std::make_shared<MockEventListener>(
                        EventListener::Type::kJSClosureEventListener, "1"));
  ASSERT_TRUE(map_->Contains("test"));

  map_->Find("test")->clear();
  EXPECT_FALSE(map_->Contains("test"));
  EXPECT_TRUE(map_->IsEmpty());
  EXPECT_TRUE(map_->Remove("test"));
  EXPECT_EQ(map_->Find("test"), nullptr);
}

TEST_F(EventListenerMapTest, RemoveLastListenerErasesOnlyMatchingEventType) {
  auto first = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "1");
  auto second = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "2");
  auto other = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "other");
  auto third = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "third");
  map_->Add("test", first);
  map_->Add("test", second);
  map_->Add("other", other);
  map_->Add("third", third);

  EXPECT_FALSE(map_->Remove("test", other));
  EXPECT_EQ(map_->Find("test")->size(), 2U);
  EXPECT_TRUE(map_->Remove("test", first));
  EXPECT_TRUE(first->removed());
  EXPECT_TRUE(map_->Contains("test"));
  EXPECT_TRUE(map_->Remove("test", second));
  EXPECT_TRUE(second->removed());
  EXPECT_FALSE(map_->Contains("test"));
  EXPECT_EQ(map_->Find("test"), nullptr);
  EXPECT_FALSE(map_->Remove("test", second));
  EXPECT_TRUE(map_->Contains("other"));
  EXPECT_TRUE(map_->Contains("third"));
  EXPECT_FALSE(other->removed());
  EXPECT_FALSE(third->removed());

  auto replacement = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "replacement");
  EXPECT_TRUE(map_->Add("test", replacement));
  EXPECT_TRUE(map_->Contains("test"));
  ASSERT_EQ(map_->Find("test")->size(), 1U);
  EXPECT_EQ(map_->Find("test")->front(), replacement);

  EXPECT_TRUE(map_->Remove("other"));
  EXPECT_TRUE(other->removed());
  EXPECT_FALSE(map_->Contains("other"));
  EXPECT_EQ(map_->Find("other"), nullptr);
  EXPECT_TRUE(map_->Contains("test"));
  EXPECT_TRUE(map_->Contains("third"));
}

TEST_F(EventListenerMapTest, RemoveAllMarksListenersAsRemoved) {
  auto listener_1 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "1");
  auto listener_2 = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "2");
  map_->Add("test-1", listener_1);
  map_->Add("test-2", listener_2);

  map_->RemoveAll();

  EXPECT_TRUE(listener_1->removed());
  EXPECT_TRUE(listener_2->removed());
  EXPECT_TRUE(map_->IsEmpty());
  EXPECT_FALSE(map_->Contains("test-1"));
  EXPECT_FALSE(map_->Contains("test-2"));
}

}  // namespace test
}  // namespace event
}  // namespace lynx
