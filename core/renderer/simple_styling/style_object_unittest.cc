// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/simple_styling/style_object.h"

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/value/ref_counted_class.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx::style {

TEST(StyleObjectTest, StyleObjectRefTypeTest) {
  tasm::StyleMap style_map;
  const fml::RefPtr<lepus::RefCounted> style_property_map_ref =
      fml::MakeRefCounted<StyleObject>(std::move(style_map));

  EXPECT_EQ(lepus::RefType::kStyleObject, style_property_map_ref->GetRefType());
}

}  // namespace lynx::style
