// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/capabilities/ios_animation_capabilities_generated.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace gfx {
namespace {

TEST(IOSAnimationCapabilitiesTest, DeclaresKeyframeAndTransitionProperties) {
  const auto capabilities = GetIOSAnimationBackendCapabilities();

  EXPECT_EQ(capabilities.backend, AnimationBackendType::kIOS);
  EXPECT_EQ(capabilities.properties.size(), 4u);
  for (const auto kind :
       {AnimationKind::kKeyframe, AnimationKind::kTransition}) {
    EXPECT_NE(capabilities.FindProperty(kind, AnimationPropertyType::kOpacity,
                                        KeyframeValueType::kFloat),
              nullptr);
    EXPECT_NE(capabilities.FindProperty(kind, AnimationPropertyType::kTransform,
                                        KeyframeValueType::kTransform),
              nullptr);
  }
}

TEST(IOSAnimationCapabilitiesTest, DeclaresNormalizedTimingFunctions) {
  const auto capabilities = GetIOSAnimationBackendCapabilities();
  const auto* opacity = capabilities.FindProperty(
      AnimationKind::kKeyframe, AnimationPropertyType::kOpacity,
      KeyframeValueType::kFloat);

  ASSERT_NE(opacity, nullptr);
  EXPECT_FALSE(opacity->supports_per_keyframe_timing);
  EXPECT_EQ(opacity->timing_functions,
            kTimingFunctionLinear | kTimingFunctionCubicBezier);
}

TEST(IOSAnimationCapabilitiesTest, DeclaresDetailedTransformFeatures) {
  const auto capabilities = GetIOSAnimationBackendCapabilities();
  for (const auto kind :
       {AnimationKind::kKeyframe, AnimationKind::kTransition}) {
    const auto* transform = capabilities.FindProperty(
        kind, AnimationPropertyType::kTransform, KeyframeValueType::kTransform);

    ASSERT_NE(transform, nullptr);
    EXPECT_EQ(transform->transform.translate_axes, kTransformAxesXYZ);
    EXPECT_EQ(transform->transform.translate_units, kTransformUnitNumber);
    EXPECT_EQ(transform->transform.rotate_axes, kTransformAxesXYZ);
    EXPECT_EQ(transform->transform.scale_axes, kTransformAxesXY);
    EXPECT_EQ(transform->transform.skew_axes, kTransformAxesXY);
    EXPECT_EQ(transform->transform.matrix_dimensions, kAllTransformMatrices);
  }
}

}  // namespace
}  // namespace gfx
}  // namespace lynx
