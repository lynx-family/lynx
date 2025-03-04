// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/concurrent_queue.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <thread>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace {

static void ToStep(std::atomic<int32_t>& s, int32_t next) {
  int32_t pre = next - 1;
  while (!s.compare_exchange_weak(pre, next)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

static void Wait(std::atomic<int32_t>& s, int32_t next) {
  while (s.load() != next) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

template <typename PopFunction>
static void TestEnqueue(ConcurrentQueue<int32_t>& queue, PopFunction pop_fn) {
  constexpr int32_t thread_num = 8;
  constexpr int32_t push_num = 100;
  std::thread threads[thread_num];
  std::atomic<int32_t> start(0);

  for (auto i = 0; i < thread_num; ++i) {
    threads[i] = std::thread([&start, &queue, i] {
      Wait(start, 1);
      for (auto j = i * push_num; j < (i + 1) * push_num; ++j) {
        queue.Push(j);
        std::this_thread::yield();
      }
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(thread_num));
  ToStep(start, 1);

  for (auto i = 0; i < thread_num; ++i) {
    threads[i].join();
  }

  auto list = (queue.*pop_fn)();

  for (auto i = 0; i < thread_num * push_num; ++i) {
    // Each push is popped
    EXPECT_TRUE(std::find(list.begin(), list.end(), i) != list.end());
  }

  for (auto i = 0; i < thread_num; ++i) {
    for (auto j = 0; j < push_num; ++j) {
      auto j_it = std::find(list.begin(), list.end(), i * push_num + j);
      for (auto j_after = i * push_num + j + 1; j_after < (i + 1) * push_num;
           ++j_after) {
        auto j_after_it = std::find(list.begin(), list.end(), j_after);
        // All push in the same thread keep the order
        EXPECT_GT(std::distance(list.begin(), j_after_it),
                  std::distance(list.begin(), j_it));
      }
    }
  }
}

TEST(ConcurrentQueueTest, ConcurrentlyEnqueueVector) {
  ConcurrentQueue<int32_t> queue;
  TestEnqueue(queue, &ConcurrentQueue<int32_t>::PopAll);
}

template <typename PopFunction>
static void TestEnqueueWithReverseDequeue(ConcurrentQueue<int32_t>& queue,
                                          PopFunction pop_fn) {
  constexpr int32_t thread_num = 8;
  constexpr int32_t push_num = 100;
  std::thread threads[thread_num];
  std::atomic<int32_t> start(0);

  for (auto i = 0; i < thread_num; ++i) {
    threads[i] = std::thread([&start, &queue, i] {
      Wait(start, 1);
      for (auto j = i * push_num; j < (i + 1) * push_num; ++j) {
        queue.Push(j);
        std::this_thread::yield();
      }
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(thread_num));
  ToStep(start, 1);

  for (auto i = 0; i < thread_num; ++i) {
    threads[i].join();
  }

  auto list = (queue.*pop_fn)();

  for (auto i = 0; i < thread_num * push_num; ++i) {
    // Each push is popped
    EXPECT_TRUE(std::find(list.begin(), list.end(), i) != list.end());
  }

  for (auto i = 0; i < thread_num; ++i) {
    for (auto j = 0; j < push_num; ++j) {
      auto j_it = std::find(list.begin(), list.end(), i * push_num + j);
      for (auto j_after = i * push_num + j + 1; j_after < (i + 1) * push_num;
           ++j_after) {
        auto j_after_it = std::find(list.begin(), list.end(), j_after);
        // All push in the same thread is popped in reverse order
        EXPECT_GT(std::distance(list.begin(), j_it),
                  std::distance(list.begin(), j_after_it));
      }
    }
  }
}

TEST(ConcurrentQueueTest, ConcurrentlyEnqueueVectorWithReverseDequeue) {
  ConcurrentQueue<int32_t> queue;
  TestEnqueueWithReverseDequeue(queue,
                                &ConcurrentQueue<int32_t>::ReversePopAll);
}

template <typename PopFunction>
static void TestAppendOrder(ConcurrentQueue<int32_t>& super_queue,
                            PopFunction pop_fn) {
  super_queue.Push(0);
  super_queue.Push(1);
  ConcurrentQueue<int32_t> sub_queue;
  sub_queue.Push(2);
  sub_queue.Push(3);

  super_queue.Push(sub_queue);
  super_queue.Push(4);

  auto result = (super_queue.*pop_fn)();
  EXPECT_EQ(result.size(), size_t(5));
  size_t i = 0;
  for (auto it = result.begin(); it != result.end(); ++it, ++i) {
    EXPECT_EQ(size_t(*it), i);
  }

  EXPECT_TRUE(sub_queue.PopAll().empty());
  EXPECT_TRUE(super_queue.PopAll().empty());
}

TEST(ConcurrentQueueTest, AppendOrderVector) {
  ConcurrentQueue<int32_t> super_queue;
  TestAppendOrder(super_queue, &ConcurrentQueue<int32_t>::PopAll);
}

template <typename PopFunction>
static void TestAppendEmpty(ConcurrentQueue<int32_t>& super_queue,
                            PopFunction pop_fn) {
  super_queue.Push(0);
  ConcurrentQueue<int32_t> sub_queue;
  super_queue.Push(sub_queue);

  auto result = (super_queue.*pop_fn)();
  EXPECT_EQ(result.size(), size_t(1));
  EXPECT_EQ(result.front(), 0);

  EXPECT_TRUE(sub_queue.PopAll().empty());
  EXPECT_TRUE(super_queue.PopAll().empty());

  sub_queue.Push(0);
  super_queue.Push(sub_queue);

  result = (super_queue.*pop_fn)();
  EXPECT_EQ(result.size(), size_t(1));
  EXPECT_EQ(result.front(), 0);

  EXPECT_TRUE(sub_queue.PopAll().empty());
  EXPECT_TRUE(super_queue.PopAll().empty());
}

TEST(ConcurrentQueueTest, AppendEmptyVector) {
  ConcurrentQueue<int32_t> super_queue;
  TestAppendEmpty(super_queue, &ConcurrentQueue<int32_t>::PopAll);
}

template <typename PopFunction>
static void TestMove(ConcurrentQueue<int32_t>& src_queue, PopFunction pop_fn) {
  src_queue.Push(0);
  ConcurrentQueue<int32_t> dst_queue(std::move(src_queue));
  auto result = (dst_queue.*pop_fn)();
  EXPECT_EQ(result.size(), size_t(1));
  EXPECT_EQ(result.front(), 0);
  EXPECT_TRUE(src_queue.PopAll().empty());
  EXPECT_TRUE(dst_queue.PopAll().empty());

  src_queue.Push(0);
  dst_queue = std::move(src_queue);
  auto result2 = (dst_queue.*pop_fn)();
  EXPECT_EQ(result2.size(), size_t(1));
  EXPECT_EQ(result2.front(), 0);
  EXPECT_TRUE(src_queue.PopAll().empty());
  EXPECT_TRUE(dst_queue.PopAll().empty());
}

TEST(ConcurrentQueueTest, MoveVector) {
  ConcurrentQueue<int32_t> src_queue;
  TestMove(src_queue, &ConcurrentQueue<int32_t>::PopAll);
}

}  // namespace
}  // namespace base
}  // namespace lynx
