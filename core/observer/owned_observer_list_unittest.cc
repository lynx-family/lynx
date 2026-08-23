// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/observer/owned_observer_list.h"

#include <memory>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace observer {
namespace {

class TestObserver {
 public:
  explicit TestObserver(int value) : value_(value) {}
  virtual ~TestObserver() = default;

  virtual int value() const { return value_; }

 private:
  int value_;
};

TEST(OwnedObserverListTest, IgnoresNullAndPreservesInsertionOrder) {
  OwnedObserverList<TestObserver> observers;
  observers.Add(nullptr);
  observers.Add(std::make_unique<TestObserver>(1));
  observers.Add(std::make_unique<TestObserver>(2));

  std::vector<int> values;
  observers.ForEach([&values](TestObserver& observer) {
    values.push_back(observer.value());
  });

  EXPECT_EQ(observers.size(), 2u);
  EXPECT_EQ(values, (std::vector<int>{1, 2}));
}

TEST(OwnedObserverListTest, MovesOwnership) {
  OwnedObserverList<TestObserver> observers;
  observers.Add(std::make_unique<TestObserver>(3));

  OwnedObserverList<TestObserver> moved = std::move(observers);

  EXPECT_EQ(moved.size(), 1u);
  int value = 0;
  moved.ForEach([&value](TestObserver& observer) { value = observer.value(); });
  EXPECT_EQ(value, 3);
}

}  // namespace
}  // namespace observer
}  // namespace lynx
