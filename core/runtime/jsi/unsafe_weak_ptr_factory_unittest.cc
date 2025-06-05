// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/unsafe_weak_ptr_factory.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {

class MyObject : public UnsafeWeakPtrFactory<MyObject> {
 public:
  MyObject(int v) : value(v) {}
  ~MyObject() {}
  int GetValue() const { return value; }

 private:
  int value;
};

class MyConstObject : public UnsafeWeakPtrFactory<const MyConstObject> {
 public:
  MyConstObject(int v) : value(v) {}
  int GetValue() const { return value; }

 private:
  int value;
};

TEST(UnsafeWeakPtrFactoryTest, BasicLifecycle) {
  MyObject* obj = new MyObject(10);
  UnsafeWeakPtr<MyObject> wp = obj->WeakFromThis();

  ASSERT_NE(wp.Lock(), nullptr);
  EXPECT_EQ(wp.Lock()->GetValue(), 10);
  EXPECT_FALSE(wp.Expired());

  delete obj;

  EXPECT_EQ(wp.Lock(), nullptr);
  EXPECT_TRUE(wp.Expired());
}

TEST(UnsafeWeakPtrFactoryTest, DefaultConstructor) {
  UnsafeWeakPtr<MyObject> wp;
  EXPECT_EQ(wp.Lock(), nullptr);
  EXPECT_TRUE(wp.Expired());
}

TEST(UnsafeWeakPtrFactoryTest, CopyConstructor) {
  MyObject* obj = new MyObject(20);
  UnsafeWeakPtr<MyObject> wp1 = obj->WeakFromThis();
  UnsafeWeakPtr<MyObject> wp2 = wp1;

  ASSERT_NE(wp1.Lock(), nullptr);
  EXPECT_EQ(wp1.Lock()->GetValue(), 20);
  ASSERT_NE(wp2.Lock(), nullptr);
  EXPECT_EQ(wp2.Lock()->GetValue(), 20);

  delete obj;

  EXPECT_EQ(wp1.Lock(), nullptr);
  EXPECT_EQ(wp2.Lock(), nullptr);
}

TEST(UnsafeWeakPtrFactoryTest, MoveConstructor) {
  MyObject* obj = new MyObject(30);
  UnsafeWeakPtr<MyObject> wp1 = obj->WeakFromThis();
  UnsafeWeakPtr<MyObject> wp2 = std::move(wp1);

  EXPECT_EQ(wp1.Lock(), nullptr);
  EXPECT_TRUE(wp1.Expired());
  ASSERT_NE(wp2.Lock(), nullptr);
  EXPECT_EQ(wp2.Lock()->GetValue(), 30);

  delete obj;
  EXPECT_EQ(wp2.Lock(), nullptr);
}

TEST(UnsafeWeakPtrFactoryTest, CopyAssignment) {
  MyObject* obj1 = new MyObject(40);
  MyObject* obj2 = new MyObject(50);

  UnsafeWeakPtr<MyObject> wp1 = obj1->WeakFromThis();
  UnsafeWeakPtr<MyObject> wp2 = obj2->WeakFromThis();
  UnsafeWeakPtr<MyObject> wp3;

  wp3 = wp1;
  ASSERT_NE(wp3.Lock(), nullptr);
  EXPECT_EQ(wp3.Lock()->GetValue(), 40);

  wp3 = wp2;
  ASSERT_NE(wp3.Lock(), nullptr);
  EXPECT_EQ(wp3.Lock()->GetValue(), 50);

  delete obj1;  // wp1's target is gone, wp3 now points to obj2's control block
  ASSERT_NE(wp3.Lock(), nullptr);  // Should still be valid as it points to obj2
  EXPECT_EQ(wp3.Lock()->GetValue(), 50);

  delete obj2;  // Now wp3's target is also gone

  EXPECT_EQ(wp3.Lock(), nullptr);
}

TEST(UnsafeWeakPtrFactoryTest, MoveAssignment) {
  MyObject* obj = new MyObject(60);
  UnsafeWeakPtr<MyObject> wp1 = obj->WeakFromThis();
  UnsafeWeakPtr<MyObject> wp2;

  wp2 = std::move(wp1);

  EXPECT_EQ(wp1.Lock(), nullptr);
  EXPECT_TRUE(wp1.Expired());
  ASSERT_NE(wp2.Lock(), nullptr);
  EXPECT_EQ(wp2.Lock()->GetValue(), 60);

  UnsafeWeakPtr<MyObject> wp_tmp = obj->WeakFromThis();
  UnsafeWeakPtr<MyObject> wp_empty;
  wp_empty = std::move(wp_tmp);
  ASSERT_NE(wp_empty.Lock(), nullptr);
  EXPECT_EQ(wp_empty.Lock()->GetValue(), 60);
  EXPECT_EQ(wp_tmp.Lock(), nullptr);

  delete obj;
  EXPECT_EQ(wp2.Lock(), nullptr);
  EXPECT_EQ(wp_empty.Lock(), nullptr);
}

TEST(UnsafeWeakPtrFactoryTest, Reset) {
  MyObject* obj = new MyObject(70);
  UnsafeWeakPtr<MyObject> wp = obj->WeakFromThis();

  ASSERT_NE(wp.Lock(), nullptr);
  wp.Reset();
  EXPECT_EQ(wp.Lock(), nullptr);
  EXPECT_TRUE(wp.Expired());

  delete obj;
}

TEST(UnsafeWeakPtrFactoryTest, MultipleWeakPtrs) {
  MyObject* obj = new MyObject(80);
  UnsafeWeakPtr<MyObject> wp1 = obj->WeakFromThis();
  UnsafeWeakPtr<MyObject> wp2 = obj->WeakFromThis();
  UnsafeWeakPtr<MyObject> wp3 = wp1;

  ASSERT_NE(wp1.Lock(), nullptr);
  ASSERT_NE(wp2.Lock(), nullptr);
  ASSERT_NE(wp3.Lock(), nullptr);

  delete obj;

  EXPECT_EQ(wp1.Lock(), nullptr);
  EXPECT_EQ(wp2.Lock(), nullptr);
  EXPECT_EQ(wp3.Lock(), nullptr);
}

TEST(UnsafeWeakPtrFactoryTest, WeakPtrScope) {
  MyObject* obj = new MyObject(90);
  {
    UnsafeWeakPtr<MyObject> wp = obj->WeakFromThis();
    ASSERT_NE(wp.Lock(), nullptr);
  }

  EXPECT_EQ(obj->GetValue(), 90);
  delete obj;
}

TEST(UnsafeWeakPtrFactoryTest, ConstObject) {
  const MyConstObject* obj = new MyConstObject(100);
  UnsafeWeakPtr<const MyConstObject> wp = obj->WeakFromThis();

  ASSERT_NE(wp.Lock(), nullptr);
  EXPECT_EQ(wp.Lock()->GetValue(), 100);
  EXPECT_FALSE(wp.Expired());

  delete obj;

  EXPECT_EQ(wp.Lock(), nullptr);
  EXPECT_TRUE(wp.Expired());
}

TEST(UnsafeWeakPtrFactoryTest, BoolConversion) {
  UnsafeWeakPtr<MyObject> wp_empty;
  EXPECT_FALSE(wp_empty);

  MyObject* obj = new MyObject(110);
  UnsafeWeakPtr<MyObject> wp_valid = obj->WeakFromThis();
  EXPECT_TRUE(wp_valid);

  if (wp_valid) {
    EXPECT_EQ(wp_valid.Lock()->GetValue(), 110);
  } else {
    FAIL() << "wp_valid should evaluate to true in boolean context";
  }

  delete obj;
  EXPECT_FALSE(wp_valid);

  if (wp_valid) {
    FAIL() << "wp_valid should evaluate to false after object deletion";
  }
}

}  // namespace piper
}  // namespace lynx
