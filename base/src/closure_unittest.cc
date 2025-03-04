// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/closure.h"

#include <functional>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

namespace {

class ReleaseCallback {
 public:
  ReleaseCallback(std::function<void()> func,
                  std::function<void()> release_func)
      : func_(std::move(func)), release_func_(std::move(release_func)) {}
  ~ReleaseCallback() {
    if (release_func_) {
      release_func_();
    }
  }

  ReleaseCallback(const ReleaseCallback&) = delete;
  ReleaseCallback& operator=(const ReleaseCallback&) = delete;

  ReleaseCallback(ReleaseCallback&& other) {
    func_ = std::move(other.func_);
    release_func_ = std::move(other.release_func_);
    other.func_ = nullptr;
    other.release_func_ = nullptr;
  }

  ReleaseCallback& operator=(ReleaseCallback&& other) {
    func_ = std::move(other.func_);
    release_func_ = std::move(other.release_func_);
    other.func_ = nullptr;
    other.release_func_ = nullptr;
    return *this;
  }

  void operator()() { func_(); }

 private:
  std::function<void()> func_;

  std::function<void()> release_func_;
};

};  // namespace

class ClosureTest : public ::testing::Test {
 protected:
  ClosureTest() = default;
  ~ClosureTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(ClosureTest, MoveOnlyClosureCheckNull1) {
  MoveOnlyClosure closure;
  ASSERT_FALSE(closure);
  ASSERT_TRUE(closure == nullptr);
  ASSERT_FALSE(closure != nullptr);
}

TEST_F(ClosureTest, MoveOnlyClosureCheckNull2) {
  MoveOnlyClosure closure(nullptr);
  ASSERT_FALSE(closure);
  ASSERT_TRUE(closure == nullptr);
  ASSERT_FALSE(closure != nullptr);
}

TEST_F(ClosureTest, MoveOnlyClosureCheckNotNull) {
  MoveOnlyClosure closure([] {});
  ASSERT_TRUE(closure);
  ASSERT_FALSE(closure == nullptr);
  ASSERT_TRUE(closure != nullptr);
}

TEST_F(ClosureTest, MoveConstructor) {
  bool has_run = false;
  bool has_release = false;
  MoveOnlyClosure need_move_closure(
      ReleaseCallback([&has_run]() mutable { has_run = true; },
                      [&has_release]() mutable { has_release = true; }));

  auto* closure = new MoveOnlyClosure(std::move(need_move_closure));
  ASSERT_FALSE(has_run);
  ASSERT_FALSE(has_release);
  ASSERT_FALSE(need_move_closure);

  (*closure)();
  ASSERT_TRUE(has_run);
  ASSERT_FALSE(has_release);

  delete closure;
  ASSERT_TRUE(has_release);
}

TEST_F(ClosureTest, AssignmentOperator) {
  bool has_run = false;
  bool has_release = false;
  MoveOnlyClosure need_move_closure(
      ReleaseCallback([&has_run]() mutable { has_run = true; },
                      [&has_release]() mutable { has_release = true; }));

  bool old_func_has_release = false;
  MoveOnlyClosure closure(ReleaseCallback(
      [] {},
      [&old_func_has_release]() mutable { old_func_has_release = true; }));
  closure = std::move(need_move_closure);
  ASSERT_FALSE(need_move_closure);
  ASSERT_FALSE(has_run);
  ASSERT_FALSE(has_release);
  ASSERT_TRUE(old_func_has_release);

  closure();
  ASSERT_TRUE(has_run);
  ASSERT_FALSE(has_release);

  closure = nullptr;
  ASSERT_TRUE(has_release);
}

TEST_F(ClosureTest, TypedClosureBasicTest) {
  // Should support return type
  MoveOnlyClosure<int32_t> closure([]() { return 0xff; });
  ASSERT_EQ(closure(), 0xff);

  // Should support return type and multiple arguments
  MoveOnlyClosure<int32_t, int32_t, int32_t> add(
      [](int32_t a, int32_t b) { return a + b; });
  ASSERT_EQ(add(1, 2), 3);

  // Should be move-only
  static_assert(std::is_move_constructible_v<decltype(add)> &&
                std::is_move_assignable_v<decltype(add)> &&
                not std::is_copy_assignable_v<decltype(add)> &&
                not std::is_copy_constructible_v<decltype(add)>);
}

TEST_F(ClosureTest, TypedClosureForwardTest) {
  class Foo {};
  MoveOnlyClosure<void, Foo&> closure([](auto& value) {
    static_assert(std::is_same_v<decltype(value), Foo&>);
    return;
  });
  Foo foo{};
  Foo* foo_p = &foo;
  closure(foo);

  MoveOnlyClosure<Foo, Foo&&> closure_rvalue([foo_p](auto&& value) {
    static_assert(std::is_same_v<decltype(value), Foo&&>);
    EXPECT_EQ(foo_p, &value);
    return value;
  });
  foo = closure_rvalue(std::move(foo));
  EXPECT_EQ(foo_p, &foo);
}

}  // namespace base
}  // namespace lynx
