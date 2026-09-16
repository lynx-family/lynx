// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <atomic>
#include <chrono>
#include <future>
#include <memory>

#include "clay/shell/platform/windows/egl/async_preparation.h"
#include "gtest/gtest.h"

namespace clay {
namespace egl {
namespace {

std::unique_ptr<int> CreateValue() { return std::make_unique<int>(42); }
std::unique_ptr<int> FailCreation() { return nullptr; }

std::atomic<int> thread_local_destructions{0};
struct ThreadLocalResource {
  ~ThreadLocalResource() { ++thread_local_destructions; }
};
std::unique_ptr<int> CreateValueWithThreadLocalResource() {
  thread_local ThreadLocalResource resource;
  (void)resource;
  return CreateValue();
}

TEST(AsyncPreparationTest, JoinWaitsForThreadLocalDestruction) {
  const int before = thread_local_destructions.load();
  AsyncPreparation<int> preparation;
  ASSERT_TRUE(preparation.Start(CreateValueWithThreadLocalResource));
  auto result = preparation.WaitAndTake();
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(*result, 42);
  EXPECT_EQ(thread_local_destructions.load(), before + 1);
}

TEST(AsyncPreparationTest, TransfersResultOnlyOnce) {
  AsyncPreparation<int> preparation;
  EXPECT_TRUE(preparation.Start(CreateValue));
  EXPECT_TRUE(preparation.Start(FailCreation));
  auto result = preparation.WaitAndTake();
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(*result, 42);
  EXPECT_EQ(preparation.WaitAndTake(), nullptr);
  EXPECT_FALSE(preparation.Start(CreateValue));
}

TEST(AsyncPreparationTest, FailureReturnsEmptyResult) {
  AsyncPreparation<int> preparation;
  ASSERT_TRUE(preparation.Start(FailCreation));
  EXPECT_EQ(preparation.WaitAndTake(), nullptr);
}

TEST(AsyncPreparationTest, UnpreparedConsumerClosesSlot) {
  AsyncPreparation<int> preparation;
  EXPECT_EQ(preparation.WaitAndTake(), nullptr);
  EXPECT_FALSE(preparation.Start(CreateValue));
}

TEST(AsyncPreparationTest, DiscardBeforeStartIsHarmless) {
  AsyncPreparation<int> preparation;
  preparation.Discard();
  preparation.Discard();
  EXPECT_FALSE(preparation.Start(CreateValue));
}

std::atomic<int> live_resources{0};
struct Resource {
  Resource() { ++live_resources; }
  ~Resource() { --live_resources; }
};
std::unique_ptr<Resource> CreateResource() {
  return std::make_unique<Resource>();
}

TEST(AsyncPreparationTest, DiscardJoinsAndReleasesUnusedResource) {
  AsyncPreparation<Resource> preparation;
  ASSERT_TRUE(preparation.Start(CreateResource));
  preparation.Discard();
  EXPECT_EQ(live_resources, 0);
  EXPECT_EQ(preparation.WaitAndTake(), nullptr);
}

TEST(AsyncPreparationTest, DestructorJoinsUnusedWork) {
  {
    AsyncPreparation<Resource> preparation;
    ASSERT_TRUE(preparation.Start(CreateResource));
  }
  EXPECT_EQ(live_resources, 0);
}

HANDLE factory_entered;
HANDLE allow_completion;
std::unique_ptr<int> CreateBlockedValue() {
  ::SetEvent(factory_entered);
  ::WaitForSingleObject(allow_completion, INFINITE);
  return CreateValue();
}

TEST(AsyncPreparationTest, StartReturnsBeforeFactoryCompletes) {
  factory_entered = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
  allow_completion = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
  ASSERT_NE(factory_entered, nullptr);
  ASSERT_NE(allow_completion, nullptr);
  AsyncPreparation<int> preparation;
  ASSERT_TRUE(preparation.Start(CreateBlockedValue));
  EXPECT_EQ(::WaitForSingleObject(factory_entered, 5000), WAIT_OBJECT_0);
  auto consumer =
      std::async(std::launch::async, [&] { return preparation.WaitAndTake(); });
  EXPECT_EQ(consumer.wait_for(std::chrono::milliseconds(20)),
            std::future_status::timeout);
  ::SetEvent(allow_completion);
  auto result = consumer.get();
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(*result, 42);
  ::CloseHandle(factory_entered);
  ::CloseHandle(allow_completion);
}

TEST(AsyncPreparationTest, ConcurrentConsumersGetExactlyOneResult) {
  AsyncPreparation<int> preparation;
  ASSERT_TRUE(preparation.Start(CreateValue));
  auto consumer =
      std::async(std::launch::async, [&] { return preparation.WaitAndTake(); });
  auto result = preparation.WaitAndTake();
  auto other_result = consumer.get();
  EXPECT_NE(static_cast<bool>(result), static_cast<bool>(other_result));
}

}  // namespace
}  // namespace egl
}  // namespace clay
