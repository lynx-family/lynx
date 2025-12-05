// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/attributes_testing.h"
#include "clay/gfx/testing_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkScalar.h"

namespace clay {
namespace testing {

TEST(DisplayListPathEffect, EffectShared) {
  const SkScalar TestDashes2[] = {1.0, 1.5};
  auto effect = DlDashPathEffect::Make(TestDashes2, 2, 0.0);
  ASSERT_TRUE(Equals(effect->shared(), effect));
}

TEST(DisplayListPathEffect, DashEffectAsDash) {
  const SkScalar TestDashes2[] = {1.0, 1.5};
  auto effect = DlDashPathEffect::Make(TestDashes2, 2, 0.0);
  ASSERT_NE(effect->asDash(), nullptr);
  ASSERT_EQ(effect->asDash(), effect.get());
}

TEST(DisplayListPathEffect, DashEffectEquals) {
  const SkScalar TestDashes2[] = {1.0, 1.5};
  auto effect1 = DlDashPathEffect::Make(TestDashes2, 2, 0.0);
  auto effect2 = DlDashPathEffect::Make(TestDashes2, 2, 0.0);
  TestEquals(*effect1, *effect1);
}

TEST(DisplayListPathEffect, CheckEffectProperties) {
  const SkScalar test_dashes[] = {4.0, 2.0};
  const SkScalar TestDashes2[] = {5.0, 2.0};
  const SkScalar TestDashes3[] = {4.0, 3.0};
  const SkScalar TestDashes4[] = {4.0, 2.0, 6.0};
  auto effect1 = DlDashPathEffect::Make(test_dashes, 2, 0.0);
  auto effect2 = DlDashPathEffect::Make(TestDashes2, 2, 0.0);
  auto effect3 = DlDashPathEffect::Make(TestDashes3, 2, 0.0);
  auto effect4 = DlDashPathEffect::Make(TestDashes4, 3, 0.0);
  auto effect5 = DlDashPathEffect::Make(test_dashes, 2, 1.0);

  TestNotEquals(*effect1, *effect2, "Interval 1 differs");
  TestNotEquals(*effect1, *effect3, "Interval 2 differs");
  TestNotEquals(*effect1, *effect4, "Dash count differs");
  TestNotEquals(*effect1, *effect5, "Dash phase differs");
}

TEST(DisplayListPathEffect, UnknownConstructor) {
  const SkScalar test_dashes[] = {4.0, 2.0};
  DlUnknownPathEffect path_effect(SkDashPathEffect::Make(test_dashes, 2, 0.0));
}

TEST(DisplayListPathEffect, UnknownShared) {
  const SkScalar test_dashes[] = {4.0, 2.0};
  DlUnknownPathEffect path_effect(SkDashPathEffect::Make(test_dashes, 2, 0.0));
  ASSERT_NE(path_effect.shared().get(), &path_effect);
  ASSERT_EQ(*path_effect.shared(), path_effect);
}

TEST(DisplayListPathEffect, UnknownContents) {
  const SkScalar test_dashes[] = {4.0, 2.0};
  sk_sp<SkPathEffect> sk_effect = SkDashPathEffect::Make(test_dashes, 2, 0.0);
  DlUnknownPathEffect effect(sk_effect);
  ASSERT_EQ(effect.gr_object(), sk_effect);
  ASSERT_EQ(effect.gr_object().get(), sk_effect.get());
}

TEST(DisplayListPathEffect, UnknownEquals) {
  const SkScalar test_dashes[] = {4.0, 2.0};
  sk_sp<SkPathEffect> sk_effect = SkDashPathEffect::Make(test_dashes, 2, 0.0);
  DlUnknownPathEffect effect1(sk_effect);
  DlUnknownPathEffect effect2(sk_effect);
  TestEquals(effect1, effect1);
}

TEST(DisplayListPathEffect, UnknownNotEquals) {
  const SkScalar test_dashes[] = {4.0, 2.0};
  // Even though the effect is the same, it is a different instance
  // and we cannot currently tell them apart because the Skia
  // DashEffect::Make objects do not implement ==
  DlUnknownPathEffect path_effect1(SkDashPathEffect::Make(test_dashes, 2, 0.0));
  DlUnknownPathEffect path_effect2(SkDashPathEffect::Make(test_dashes, 2, 0.0));
  TestNotEquals(path_effect1, path_effect2,
                "SkDashPathEffect instance differs");
}

}  // namespace testing
}  // namespace clay
