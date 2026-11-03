// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/flex_optional.h"

#include <initializer_list>
#include <optional>
#include <utility>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace test {

struct s32 {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t d;
};

struct s32_mem_save {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t d;

  // A flag telling `base::flex_optional<>` to save memory.
  using AlwaysUseFlexOptionalMemSave = bool;
};

struct s40Convertible {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t d;
  uint64_t e;
};

struct s40 {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t d;
  uint64_t e;
  s40(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e)
      : a(a), b(b), c(c), d(d), e(e) {}

  s40(const s40Convertible& s) : a(s.a) {}
  template <class U>
  s40(std::initializer_list<U> list)
      : a(list.size() > 0 ? list.begin()[0] : 0),
        b(list.size() > 1 ? list.begin()[1] : 0),
        c(list.size() > 2 ? list.begin()[2] : 0),
        d(list.size() > 3 ? list.begin()[3] : 0),
        e(list.size() > 4 ? list.begin()[4] : 0) {}
};

struct s40MoveOnly {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t d;
  uint64_t e;
  s40MoveOnly(const s40MoveOnly& other) = delete;
  s40MoveOnly& operator=(const s40MoveOnly& other) = delete;
};

TEST(FlexOptional, ChooseFromType) {
  auto small = base::flex_optional<s32>();
  ASSERT_EQ(sizeof(small), sizeof(std::optional<s32>));
  auto small_mem_save = base::flex_optional<s32_mem_save>();
  ASSERT_EQ(sizeof(small_mem_save), sizeof(void*));
  auto big = base::flex_optional<s40>();
  ASSERT_EQ(sizeof(big), sizeof(void*));

  ASSERT_TRUE(!big);
  ASSERT_FALSE(big.has_value());
  big = s40{
      1, 2, 3, 4, 5,
  };

  ASSERT_FALSE(!big);
  ASSERT_TRUE(big.has_value());
  ASSERT_EQ(big->a, 1u);
  ASSERT_EQ(big.value().a, 1u);
}

TEST(FlexOptional, CopyConstructorFromEmpty) {
  base::flex_optional<s40> original;
  base::flex_optional<s40> copy(original);
  EXPECT_FALSE(bool(copy));
}

TEST(FlexOptional, CopyConstructorFromValue) {
  const base::flex_optional<s40> original(std::in_place, 1, 2, 3, 4, 5);
  const base::flex_optional<s40> copy(original);
  EXPECT_TRUE(bool(copy));
  EXPECT_EQ(copy->a, 1u);
}
TEST(FlexOptional, MoveValueConstructor) {
  base::flex_optional<s40> s40MoveOnly(std::in_place, 1, 2, 3, 4, 5);
  base::flex_optional<s40> moved(std::move(s40MoveOnly));
  EXPECT_TRUE(bool(moved));
  EXPECT_EQ(moved->a, 1u);
}

TEST(FlexOptional, NulloptConstructor) {
  base::flex_optional<s40> obj(std::nullopt);
  EXPECT_FALSE(bool(obj));
}

TEST(FlexOptional, InPlaceConstructor) {
  const base::flex_optional<s40> obj(std::in_place, 1, 2, 3, 4, 5);
  EXPECT_TRUE(bool(obj));
  EXPECT_EQ(obj->a, 1u);
}

TEST(FlexOptional, InPlaceConstructorWithInitializerList) {
  const base::flex_optional<s40> obj(std::in_place, {1, 2, 3, 4, 5});
  EXPECT_TRUE(bool(obj));
  EXPECT_EQ(obj->a, 1u);

  const base::flex_optional<std::vector<int>> vec(std::in_place,
                                                  {1, 2, 3, 4, 5});
  EXPECT_TRUE(bool(vec));
  EXPECT_EQ(vec->operator[](0), 1);
}

TEST(FlexOptional, CopyConstructorWithValue) {
  const base::flex_optional<s40> original(std::in_place, 1, 2, 3, 4, 5);
  base::flex_optional<s40> copy(original);

  EXPECT_TRUE(original.has_value());
  EXPECT_TRUE(copy.has_value());
  EXPECT_EQ(original.value().a, copy.value().a);
  EXPECT_NE(&original.value(), &copy.value());
}

TEST(FlexOptional, CopyConstructorWithoutValue) {
  base::flex_optional<s40> original;
  base::flex_optional<s40> copy(original);

  EXPECT_FALSE(original.has_value());
  EXPECT_FALSE(copy.has_value());
}

TEST(FlexOptional, MoveConstructorWithValue) {
  // const ?
  base::flex_optional<s40> original(std::in_place, 1, 2, 3, 4, 5);
  s40 original_value = original.value();
  base::flex_optional<s40> moved(std::move(original));

  EXPECT_FALSE(original.has_value());
  EXPECT_TRUE(moved.has_value());
  EXPECT_EQ(moved.value().a, original_value.a);
}

TEST(FlexOptional, MoveConstructorWithoutValue) {
  base::flex_optional<s40> original;
  base::flex_optional<s40> moved(std::move(original));

  EXPECT_FALSE(original.has_value());
  EXPECT_FALSE(moved.has_value());
}

TEST(FlexOptional, OperatorAssign) {
  base::flex_optional<s40> obj = s40{1};
  base::flex_optional<s40> from = s40{2};

  // const copy
  obj = from;
  EXPECT_EQ(obj->a, 2u);

  // move copy
  obj = base::flex_optional<s40>({1});
  EXPECT_EQ(obj->a, 1u);

  // move assign
  obj = std::move(from);
  EXPECT_EQ(obj->a, 2u);

  // nullopt
  obj = std::nullopt;
  EXPECT_FALSE(bool(obj));
}

TEST(FlexOptional, OperatorAssignConstructable) {
  base::flex_optional<s40> obj = s40{1};
  base::flex_optional<s40Convertible> from = s40Convertible{2};

  // const copy
  obj = from;
  EXPECT_EQ(obj->a, 2u);

  // move copy
  obj = base::flex_optional<s40Convertible>({1});
  EXPECT_EQ(obj->a, 1u);

  // move assign
  obj = std::move(from);
  EXPECT_EQ(obj->a, 2u);

  // nullopt
  obj = std::nullopt;
  EXPECT_FALSE(bool(obj));
}

TEST(FlexOptional, OperatorAssignByValue) {
  base::flex_optional<s40> obj = s40{1};
  s40 from = s40{2};

  // const copy
  obj = from;
  EXPECT_EQ(obj->a, 2u);

  // move copy
  obj = s40{1};
  EXPECT_EQ(obj->a, 1u);

  // move assign
  obj = std::move(from);
  EXPECT_EQ(obj->a, 2u);
}

TEST(FlexOptional, Emplace) {
  base::flex_optional<s40> obj;
  obj.emplace(1, 2, 3, 4, 5);
  EXPECT_TRUE(bool(obj));
  EXPECT_EQ(obj->a, 1u);

  base::flex_optional<std::vector<int>> vec;
  vec.emplace(5, 0);
  EXPECT_TRUE(bool(vec));
  EXPECT_EQ(vec->operator[](0), 0);
}

TEST(FlexOptional, EmplaceInitializerList) {
  base::flex_optional<s40> obj;
  obj.emplace({1, 2, 3, 4, 5});
  EXPECT_TRUE(bool(obj));
  EXPECT_EQ(obj->a, 1u);

  base::flex_optional<std::vector<int>> vec;
  vec.emplace({1, 2, 3, 4, 5});
  EXPECT_TRUE(bool(vec));
  EXPECT_EQ(vec->operator[](0), 1);
}

TEST(FlexOptional, Swap) {
  base::flex_optional<s40> obj1 = s40{1, 2, 3, 4, 5};
  base::flex_optional<s40> obj2 = s40{1, 2, 3, 4, 5};

  void* ptr1 = &*obj1;
  void* ptr2 = &*obj2;
  obj1.swap(obj2);
  EXPECT_EQ(&*obj1, ptr2);
  EXPECT_EQ(&*obj2, ptr1);
}

TEST(FlexOptional, HasValue) {
  base::flex_optional<s40> obj;
  EXPECT_FALSE(bool(obj));
  EXPECT_FALSE(obj.has_value());
  obj.emplace({1, 2, 3, 4, 5});
  EXPECT_TRUE(bool(obj));
  EXPECT_TRUE(obj.has_value());
}

TEST(FlexOptional, OperatorStar) {
  base::flex_optional<s40> obj;
  obj.emplace({1, 2, 3, 4, 5});
  EXPECT_EQ(obj->a, 1u);
  EXPECT_EQ((*obj).a, 1u);

  const base::flex_optional<s40> c_obj = s40{1, 2, 3, 4, 5};
  EXPECT_EQ(c_obj->a, 1u);
  EXPECT_EQ((*c_obj).a, 1u);
}

TEST(FlexOptional, Value) {
  base::flex_optional<s40> obj = s40{1};
  EXPECT_EQ(obj.value().a, 1u);

  const base::flex_optional<s40> c_obj = s40{1};
  EXPECT_EQ(c_obj.value().a, 1u);

  EXPECT_EQ(std::move(obj).value().a, 1u);
  EXPECT_EQ(std::move(c_obj).value().a, 1u);
}

TEST(FlexOptional, ValueOr) {
  base::flex_optional<s40> obj;
  EXPECT_EQ(obj.value_or(s40{1}).a, 1u);
  base::flex_optional<s40> obj2 = s40{2};
  EXPECT_EQ(obj2.value_or(s40{1}).a, 2u);
}

TEST(FlexOptional, Reset) {
  base::flex_optional<s40> obj = s40{1};
  obj.reset();
  EXPECT_FALSE(bool(obj));
}

TEST(FlexOptional, TypeInfer) {
  std::optional obj = s40{1};
  EXPECT_EQ(obj->a, 1u);
  EXPECT_EQ(obj.value().a, 1u);
}
// TODO: operator test

}  // namespace test
}  // namespace base
}  // namespace lynx
