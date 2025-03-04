// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/type_traits_addon.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace {

struct SimpleStruct {};

TEST(TemplateUtil, RemoveCvRefT) {
  static_assert(std::is_same<int, remove_cvref_t<const int>>::value, "");
  static_assert(std::is_same<int, remove_cvref_t<const volatile int>>::value,
                "");
  static_assert(std::is_same<int, remove_cvref_t<int&>>::value, "");
  static_assert(std::is_same<int, remove_cvref_t<const int&>>::value, "");
  static_assert(std::is_same<int, remove_cvref_t<const volatile int&>>::value,
                "");
  static_assert(std::is_same<int, remove_cvref_t<int&&>>::value, "");
  static_assert(
      std::is_same<SimpleStruct, remove_cvref_t<const SimpleStruct&>>::value,
      "");
  static_assert(std::is_same<int*, remove_cvref_t<int*>>::value, "");

  // Test references and pointers to arrays.
  static_assert(std::is_same<int[3], remove_cvref_t<int[3]>>::value, "");
  static_assert(std::is_same<int[3], remove_cvref_t<int(&)[3]>>::value, "");
  static_assert(std::is_same<int(*)[3], remove_cvref_t<int(*)[3]>>::value, "");

  // Test references and pointers to functions.
  static_assert(std::is_same<void(int), remove_cvref_t<void(int)>>::value, "");
  static_assert(std::is_same<void(int), remove_cvref_t<void (&)(int)>>::value,
                "");
  static_assert(
      std::is_same<void (*)(int), remove_cvref_t<void (*)(int)>>::value, "");
}
}  // namespace
}  // namespace base
}  // namespace lynx
