// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/memory/unsafe_owning_ptr.h"

#include <utility>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace {

struct Counter {
  int* alive;
  int value;
  explicit Counter(int* flag, int v = 0) : alive(flag), value(v) {
    if (alive) ++(*alive);
  }
  ~Counter() {
    if (alive) --(*alive);
  }
};

struct Base {
  virtual ~Base() = default;
  virtual int Tag() const { return 1; }
};

struct Derived : public Base {
  int Tag() const override { return 2; }
};

// A node that holds a self-weak to exercise the case where the managed
// object destroys its own weak ref during destruction (like
// `enable_shared_from_this`-style self references).
struct SelfWeakNode {
  UnsafeWeakPtr<SelfWeakNode> self;
  int* alive;
  explicit SelfWeakNode(int* flag) : alive(flag) {
    if (alive) ++(*alive);
  }
  ~SelfWeakNode() {
    if (alive) --(*alive);
  }
};

}  // namespace

TEST(UnsafeOwningPtrTest, DefaultConstructIsNull) {
  UnsafeOwningPtr<int> ptr;
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(ptr));
  EXPECT_TRUE(ptr == nullptr);
}

TEST(UnsafeOwningPtrTest, MakeUnsafeOwningCreatesObject) {
  int alive = 0;
  {
    auto ptr = MakeUnsafeOwning<Counter>(&alive, 42);
    EXPECT_TRUE(static_cast<bool>(ptr));
    EXPECT_NE(ptr.get(), nullptr);
    EXPECT_EQ(ptr->value, 42);
    EXPECT_EQ(alive, 1);
  }
  EXPECT_EQ(alive, 0);
}

TEST(UnsafeOwningPtrTest, MoveTransfersOwnership) {
  int alive = 0;
  auto a = MakeUnsafeOwning<Counter>(&alive, 7);
  Counter* raw = a.get();
  EXPECT_EQ(alive, 1);

  UnsafeOwningPtr<Counter> b = std::move(a);
  EXPECT_EQ(a.get(), nullptr);
  EXPECT_EQ(b.get(), raw);
  EXPECT_EQ(alive, 1);

  b.Reset();
  EXPECT_EQ(alive, 0);
}

TEST(UnsafeOwningPtrTest, ResetDestroysObject) {
  int alive = 0;
  auto ptr = MakeUnsafeOwning<Counter>(&alive);
  EXPECT_EQ(alive, 1);
  ptr.Reset();
  EXPECT_EQ(alive, 0);
  EXPECT_EQ(ptr.get(), nullptr);
}

TEST(UnsafeOwningPtrTest, ReleaseGivesUpOwnership) {
  int alive = 0;
  auto ptr = MakeUnsafeOwning<Counter>(&alive);
  Counter* raw = ptr.Release();
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_EQ(alive, 1);
  delete raw;
  EXPECT_EQ(alive, 0);
}

TEST(UnsafeWeakPtrTest, EmptyByDefault) {
  UnsafeWeakPtr<int> weak;
  EXPECT_TRUE(weak.Expired());
  EXPECT_EQ(weak.Lock(), nullptr);
}

TEST(UnsafeWeakPtrTest, ObservesOwnerLifetime) {
  int alive = 0;
  auto owner = MakeUnsafeOwning<Counter>(&alive, 9);
  UnsafeWeakPtr<Counter> weak = owner.GetWeakPtr();
  EXPECT_FALSE(weak.Expired());
  EXPECT_EQ(weak.Lock(), owner.get());
  EXPECT_EQ(weak.Lock()->value, 9);

  owner.Reset();
  EXPECT_TRUE(weak.Expired());
  EXPECT_EQ(weak.Lock(), nullptr);
  EXPECT_EQ(alive, 0);
}

TEST(UnsafeWeakPtrTest, MultipleWeakShareControlBlock) {
  int alive = 0;
  auto owner = MakeUnsafeOwning<Counter>(&alive);
  UnsafeWeakPtr<Counter> w1 = owner.GetWeakPtr();
  UnsafeWeakPtr<Counter> w2 = owner.GetWeakPtr();
  UnsafeWeakPtr<Counter> w3 = w1;
  EXPECT_FALSE(w1.Expired());
  EXPECT_FALSE(w2.Expired());
  EXPECT_FALSE(w3.Expired());

  owner.Reset();
  EXPECT_TRUE(w1.Expired());
  EXPECT_TRUE(w2.Expired());
  EXPECT_TRUE(w3.Expired());
  EXPECT_EQ(alive, 0);
}

TEST(UnsafeWeakPtrTest, ResetReleasesObservation) {
  int alive = 0;
  auto owner = MakeUnsafeOwning<Counter>(&alive);
  UnsafeWeakPtr<Counter> weak = owner.GetWeakPtr();
  weak.Reset();
  EXPECT_TRUE(weak.Expired());
  EXPECT_EQ(weak.Lock(), nullptr);
  EXPECT_EQ(alive, 1);
  owner.Reset();
  EXPECT_EQ(alive, 0);
}

TEST(UnsafeOwningPtrTest, DerivedToBaseMoveConversion) {
  UnsafeOwningPtr<Derived> d = MakeUnsafeOwning<Derived>();
  Derived* raw = d.get();
  UnsafeOwningPtr<Base> b = std::move(d);
  EXPECT_EQ(d.get(), nullptr);
  EXPECT_EQ(b.get(), raw);
  EXPECT_EQ(b->Tag(), 2);
}

TEST(UnsafeWeakPtrTest, DerivedToBaseWeakConversion) {
  UnsafeOwningPtr<Derived> d = MakeUnsafeOwning<Derived>();
  UnsafeWeakPtr<Derived> weak_d = d.GetWeakPtr();
  UnsafeWeakPtr<Base> weak_b = weak_d;
  EXPECT_FALSE(weak_b.Expired());
  Base* base_ptr = weak_b.Lock();
  ASSERT_NE(base_ptr, nullptr);
  EXPECT_EQ(base_ptr->Tag(), 2);

  d.Reset();
  EXPECT_TRUE(weak_b.Expired());
}

TEST(UnsafeOwningPtrTest, NullptrComparison) {
  UnsafeOwningPtr<int> a;
  EXPECT_TRUE(a == nullptr);
  EXPECT_FALSE(a != nullptr);

  auto b = MakeUnsafeOwning<int>(1);
  EXPECT_FALSE(b == nullptr);
  EXPECT_TRUE(b != nullptr);
}

// Regression: the managed object holds a weak ref to itself (similar to
// `enable_shared_from_this`). Destruction must not double-free the control
// block.
TEST(UnsafeOwningPtrTest, SelfWeakInsideObjectDoesNotDoubleFree) {
  int alive = 0;
  auto owner = MakeUnsafeOwning<SelfWeakNode>(&alive);
  owner->self = owner.GetWeakPtr();
  EXPECT_EQ(alive, 1);
  owner.Reset();
  EXPECT_EQ(alive, 0);
}

// Regression: the managed object releases all outstanding weak refs during
// its destructor. The outer Reset must still tear things down safely.
TEST(UnsafeOwningPtrTest, AllWeakReleasedInsideDestructor) {
  int alive = 0;
  auto owner = MakeUnsafeOwning<SelfWeakNode>(&alive);
  // Grab an extra weak and stash it inside the node so it dies with the
  // node, not with the outer scope.
  owner->self = owner.GetWeakPtr();
  UnsafeWeakPtr<SelfWeakNode> outside = owner.GetWeakPtr();
  EXPECT_FALSE(outside.Expired());
  owner.Reset();
  EXPECT_EQ(alive, 0);
  EXPECT_TRUE(outside.Expired());
}

}  // namespace base
}  // namespace lynx
