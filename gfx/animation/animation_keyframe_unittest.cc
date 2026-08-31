// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/animation_keyframe.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace gfx {
namespace {

class TestLengthKeyframe final : public LengthKeyframe {
 public:
  TestLengthKeyframe()
      : LengthKeyframe(fml::TimeDelta::FromSecondsF(0.25), nullptr) {}

  using LengthKeyframe::ClearResolvedValue;
};

class TestVec2Keyframe final : public Vec2Keyframe {
 public:
  TestVec2Keyframe()
      : Vec2Keyframe(fml::TimeDelta::FromSecondsF(0.5), nullptr) {}

  using Vec2Keyframe::ClearResolvedValue;
};

class TestTransformKeyframe final : public TransformKeyframe {
 public:
  TestTransformKeyframe()
      : TransformKeyframe(fml::TimeDelta::FromSecondsF(0.5), nullptr) {}

  using TransformKeyframe::ClearResolvedValue;
};

class TestFilterKeyframe final : public FilterKeyframe {
 public:
  TestFilterKeyframe()
      : FilterKeyframe(fml::TimeDelta::FromSecondsF(1.0), nullptr) {}

  using FilterKeyframe::ClearResolvedValue;
};

TEST(AnimationKeyframeTest, LengthKeyframeStoresResolvedValue) {
  TestLengthKeyframe keyframe;
  EXPECT_TRUE(keyframe.IsEmpty());
  EXPECT_FALSE(keyframe.HasResolvedValue());
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(0.25), keyframe.Time());

  const LengthValue value{50.0f, LengthUnit::kPercent};
  keyframe.SetResolvedValue(value);

  EXPECT_FALSE(keyframe.IsEmpty());
  ASSERT_TRUE(keyframe.HasResolvedValue());
  EXPECT_FLOAT_EQ(keyframe.ResolvedValue().value, 50.0f);
  EXPECT_EQ(keyframe.ResolvedValue().unit, LengthUnit::kPercent);

  keyframe.ClearResolvedValue();
  EXPECT_FALSE(keyframe.HasResolvedValue());
  EXPECT_FALSE(keyframe.IsEmpty());
}

TEST(AnimationKeyframeTest, Vec2KeyframeStoresResolvedValue) {
  TestVec2Keyframe keyframe;
  EXPECT_TRUE(keyframe.IsEmpty());
  EXPECT_FALSE(keyframe.HasResolvedValue());
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(0.5), keyframe.Time());

  const Vec2Tagged value{{10.0, UnitTag::kNumber}, {25.0, UnitTag::kPercent}};
  keyframe.SetResolvedValue(value);

  EXPECT_FALSE(keyframe.IsEmpty());
  ASSERT_TRUE(keyframe.HasResolvedValue());
  EXPECT_DOUBLE_EQ(keyframe.ResolvedValue().x.value, 10.0);
  EXPECT_EQ(keyframe.ResolvedValue().x.tag, UnitTag::kNumber);
  EXPECT_DOUBLE_EQ(keyframe.ResolvedValue().y.value, 25.0);
  EXPECT_EQ(keyframe.ResolvedValue().y.tag, UnitTag::kPercent);

  keyframe.ClearResolvedValue();
  EXPECT_FALSE(keyframe.HasResolvedValue());
  EXPECT_FALSE(keyframe.IsEmpty());
}

TEST(AnimationKeyframeTest, TransformKeyframeStoresResolvedValue) {
  TestTransformKeyframe keyframe;
  EXPECT_TRUE(keyframe.IsEmpty());
  EXPECT_FALSE(keyframe.HasResolvedValue());
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(0.5), keyframe.Time());

  TransformOperations operations;
  operations.AppendScale(2.0f, 3.0f);
  keyframe.SetResolvedValue(operations);

  EXPECT_FALSE(keyframe.IsEmpty());
  ASSERT_TRUE(keyframe.HasResolvedValue());
  EXPECT_TRUE(keyframe.ResolvedValue().ApproximatelyEqual(
      operations, TransformOperations::kApproximatelyEqualTolerance));

  keyframe.ClearResolvedValue();
  EXPECT_FALSE(keyframe.HasResolvedValue());
  EXPECT_FALSE(keyframe.IsEmpty());
}

TEST(AnimationKeyframeTest, FilterKeyframeStoresResolvedValue) {
  TestFilterKeyframe keyframe;
  EXPECT_TRUE(keyframe.IsEmpty());
  EXPECT_FALSE(keyframe.HasResolvedValue());
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(1.0), keyframe.Time());

  const FilterValue value{3u, 0.75, UnitTag::kNumber};
  keyframe.SetResolvedValue(value);

  EXPECT_FALSE(keyframe.IsEmpty());
  ASSERT_TRUE(keyframe.HasResolvedValue());
  EXPECT_EQ(keyframe.ResolvedValue().function, 3u);
  EXPECT_DOUBLE_EQ(keyframe.ResolvedValue().value, 0.75);
  EXPECT_EQ(keyframe.ResolvedValue().unit, UnitTag::kNumber);

  keyframe.ClearResolvedValue();
  EXPECT_FALSE(keyframe.HasResolvedValue());
  EXPECT_FALSE(keyframe.IsEmpty());
}

TEST(AnimationKeyframeTest, SimpleKeyframesAreDirectlyUsable) {
  auto float_keyframe = FloatKeyframe::Create(fml::TimeDelta());
  EXPECT_TRUE(float_keyframe->IsEmpty());
  float_keyframe->SetValue(0.5f);
  EXPECT_FALSE(float_keyframe->IsEmpty());
  EXPECT_FLOAT_EQ(float_keyframe->Value(), 0.5f);

  auto color_keyframe = ColorKeyframe::Create(fml::TimeDelta());
  color_keyframe->SetValue(0xFF00FF00);
  EXPECT_EQ(color_keyframe->Value(), 0xFF00FF00u);

  auto int_keyframe = IntKeyframe::Create(fml::TimeDelta());
  int_keyframe->SetValue(2);
  EXPECT_EQ(int_keyframe->Value(), 2);
}

}  // namespace
}  // namespace gfx
}  // namespace lynx
