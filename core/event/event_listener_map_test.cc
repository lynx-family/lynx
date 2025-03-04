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

  // clear
  map_->Clear();
  EXPECT_TRUE(map_->IsEmpty());
  EXPECT_FALSE(map_->Contains("test"));
  vec_ptr = map_->Find("test");
  EXPECT_EQ(vec_ptr, nullptr);
  EXPECT_FALSE(map_->Remove(
      "test", std::make_unique<MockEventListener>(
                  EventListener::Type::kJSClosureEventListener, "1")));
}

}  // namespace test
}  // namespace event
}  // namespace lynx
