/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// https://cs.android.com/android/platform/superproject/+/master:frameworks/native/services/inputflinger/tests/BlockingQueue_test.cpp

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/thread/blocking_queue.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(BlockingQueueTest, Queue_AddAndRemove) {
  constexpr size_t capacity = 10;
  BlockingQueue<int> queue(capacity);

  queue.Push(1);
  ASSERT_EQ(queue.Pop(), 1);
}

TEST(BlockingQueueTest, Queue_isFIFO) {
  constexpr size_t capacity = 10;
  BlockingQueue<int> queue(capacity);

  for (size_t i = 0; i < capacity; i++) {
    queue.Push(static_cast<int>(i));
  }
  for (size_t i = 0; i < capacity; i++) {
    ASSERT_EQ(queue.Pop(), static_cast<int>(i));
  }
}

TEST(BlockingQueueTest, Queue_AllowsMultipleThreads) {
  constexpr size_t capacity =
      100;  // large capacity to increase likelihood that threads overlap
  BlockingQueue<int> queue(capacity);

  // Fill queue from a different thread
  std::thread fillQueue([&queue]() {
    for (size_t i = 0; i < capacity; i++) {
      queue.Push(static_cast<int>(i));
    }
  });

  // Make sure all elements are received in correct order
  for (size_t i = 0; i < capacity; i++) {
    ASSERT_EQ(queue.Pop(), static_cast<int>(i));
  }

  fillQueue.join();
}

TEST(BlockingQueueTest, Queue_BlocksWhileWaitingForElements) {
  constexpr size_t capacity = 1;
  BlockingQueue<int> queue(capacity);

  std::atomic_bool hasReceivedElement = false;

  // fill queue from a different thread
  std::thread waitUntilHasElements([&queue, &hasReceivedElement]() {
    queue.Pop();  // This should block until an element has been added
    hasReceivedElement = true;
  });

  ASSERT_FALSE(hasReceivedElement);
  queue.Push(1);
  waitUntilHasElements.join();
  ASSERT_TRUE(hasReceivedElement);
}

TEST(BlockingQueueTest, Queue_BlocksWhileWaitingForSpace) {
  constexpr size_t capacity = 1;
  BlockingQueue<int> queue(capacity);

  queue.Push(0);  // fill queue

  std::atomic_bool hasPushedElement = false;

  // push queue from a different thread
  std::thread waitUntilHasSpace([&queue, &hasPushedElement]() {
    queue.Push(1);  // This should block until queue is not full
    hasPushedElement = true;
  });

  ASSERT_FALSE(hasPushedElement);
  queue.Pop();
  waitUntilHasSpace.join();
  ASSERT_TRUE(hasPushedElement);
}

}  // namespace base
}  // namespace lynx
