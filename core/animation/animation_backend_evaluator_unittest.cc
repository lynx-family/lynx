// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/animation_backend_evaluator.h"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "gfx/animation/animation_keyframe.h"
#include "gfx/animation/capabilities/ios_animation_capabilities_generated.h"
#include "gfx/animation/timing_function.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace animation {
namespace {

gfx::AnimationBackendCapabilities MakeIOSCapabilities() {
  return gfx::GetIOSAnimationBackendCapabilities();
}

gfx::AnimationPropertyCapability* FindTransformCapability(
    gfx::AnimationBackendCapabilities& capabilities,
    gfx::AnimationKind kind = gfx::AnimationKind::kKeyframe) {
  auto iter = std::find_if(
      capabilities.properties.begin(), capabilities.properties.end(),
      [kind](const auto& capability) {
        return capability.kind == kind &&
               capability.property == gfx::AnimationPropertyType::kTransform;
      });
  return iter == capabilities.properties.end() ? nullptr : &*iter;
}

std::vector<std::unique_ptr<gfx::Keyframe>> MakeOpacityKeyframes(
    std::unique_ptr<gfx::TimingFunction> start_timing = nullptr) {
  std::vector<std::unique_ptr<gfx::Keyframe>> keyframes;
  auto start = gfx::FloatKeyframe::Create(fml::TimeDelta::Zero(),
                                          std::move(start_timing));
  start->SetValue(0.0f);
  keyframes.push_back(std::move(start));
  auto end =
      gfx::FloatKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  end->SetValue(1.0f);
  keyframes.push_back(std::move(end));
  return keyframes;
}

std::vector<std::unique_ptr<gfx::Keyframe>> MakeColorKeyframes() {
  std::vector<std::unique_ptr<gfx::Keyframe>> keyframes;
  auto start = gfx::ColorKeyframe::Create(fml::TimeDelta::Zero(),
                                          gfx::LinearTimingFunction::Create());
  start->SetValue(0xff000000);
  keyframes.push_back(std::move(start));
  auto end = gfx::ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0),
                                        gfx::LinearTimingFunction::Create());
  end->SetValue(0xffffffff);
  keyframes.push_back(std::move(end));
  return keyframes;
}

std::vector<std::unique_ptr<gfx::Keyframe>> MakeTransformKeyframes(
    gfx::TransformOperations start_operations,
    gfx::TransformOperations end_operations) {
  std::vector<std::unique_ptr<gfx::Keyframe>> keyframes;
  auto start = gfx::TransformKeyframe::Create(fml::TimeDelta::Zero());
  start->SetResolvedValue(std::move(start_operations));
  keyframes.push_back(std::move(start));
  auto end = gfx::TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0));
  end->SetResolvedValue(std::move(end_operations));
  keyframes.push_back(std::move(end));
  return keyframes;
}

std::vector<std::unique_ptr<gfx::Keyframe>> MakeTransformKeyframes(
    gfx::LengthUnit translate_unit = gfx::LengthUnit::kNumber) {
  gfx::TransformOperations start_operations;
  start_operations.AppendTranslate({0.0f, translate_unit}, {}, {});
  gfx::TransformOperations end_operations;
  end_operations.AppendTranslate({100.0f, translate_unit}, {}, {});
  return MakeTransformKeyframes(std::move(start_operations),
                                std::move(end_operations));
}

AnimationBackendRequest MakeRequest(
    const std::vector<std::unique_ptr<gfx::Keyframe>>& keyframes,
    gfx::AnimationPropertyType property =
        gfx::AnimationPropertyType::kOpacity) {
  AnimationBackendRequest request;
  request.kind = gfx::AnimationKind::kKeyframe;
  request.property = property;
  request.keyframes.reserve(keyframes.size());
  for (const auto& keyframe : keyframes) {
    request.keyframes.push_back(keyframe.get());
  }
  return request;
}

TEST(AnimationBackendEvaluatorTest, IOSRejectsUnadvertisedColorProperties) {
  auto keyframes = MakeColorKeyframes();

  for (const auto property : {gfx::AnimationPropertyType::kBackgroundColor,
                              gfx::AnimationPropertyType::kColor}) {
    const auto result = EvaluateAnimationBackend(
        MakeRequest(keyframes, property), MakeIOSCapabilities());
    EXPECT_EQ(result.fallback_reason,
              AnimationFallbackReason::kUnsupportedProperty);
  }
}

TEST(AnimationBackendEvaluatorTest, ChecksOnlyRequiredEventsForEachKind) {
  auto keyframes = MakeOpacityKeyframes();
  auto request = MakeRequest(keyframes);
  auto capabilities = MakeIOSCapabilities();
  request.required_events = gfx::kAnimationEventStart |
                            gfx::kAnimationEventEnd |
                            gfx::kAnimationEventCancel;
  EXPECT_TRUE(EvaluateAnimationBackend(request, capabilities).CanRun());

  request.required_events |= gfx::kAnimationEventIteration;
  EXPECT_EQ(EvaluateAnimationBackend(request, capabilities).fallback_reason,
            AnimationFallbackReason::kUnsupportedEvent);
  capabilities.event_capabilities.front().events |=
      gfx::kAnimationEventIteration;
  EXPECT_TRUE(EvaluateAnimationBackend(request, capabilities).CanRun());

  request.kind = gfx::AnimationKind::kTransition;
  request.required_events = gfx::kAnimationEventStart | gfx::kAnimationEventEnd;
  EXPECT_TRUE(EvaluateAnimationBackend(request, capabilities).CanRun());
  for (const auto event :
       {gfx::kAnimationEventCancel, gfx::kAnimationEventIteration}) {
    request.required_events = event;
    EXPECT_EQ(EvaluateAnimationBackend(request, capabilities).fallback_reason,
              AnimationFallbackReason::kUnsupportedEvent)
        << "event=" << static_cast<int>(event);
  }
  capabilities.event_capabilities.clear();
  request.required_events = 0;
  EXPECT_TRUE(EvaluateAnimationBackend(request, capabilities).CanRun());
  request.required_events = gfx::kAnimationEventEnd;
  EXPECT_EQ(EvaluateAnimationBackend(request, capabilities).fallback_reason,
            AnimationFallbackReason::kUnsupportedEvent);
}

TEST(AnimationBackendEvaluatorTest, IOSRejectsTransformWithoutCapability) {
  auto keyframes = MakeTransformKeyframes();
  auto capabilities = MakeIOSCapabilities();
  const auto transform = FindTransformCapability(capabilities);
  ASSERT_NE(transform, nullptr);
  capabilities.properties.erase(capabilities.properties.begin() +
                                (transform - capabilities.properties.data()));

  const auto result = EvaluateAnimationBackend(
      MakeRequest(keyframes, gfx::AnimationPropertyType::kTransform),
      capabilities);

  EXPECT_EQ(result.fallback_reason,
            AnimationFallbackReason::kUnsupportedProperty);
}

TEST(AnimationBackendEvaluatorTest, IOSRoutesStableTransformKeyframes) {
  auto keyframes = MakeTransformKeyframes();
  for (const auto kind :
       {gfx::AnimationKind::kKeyframe, gfx::AnimationKind::kTransition}) {
    auto request =
        MakeRequest(keyframes, gfx::AnimationPropertyType::kTransform);
    request.kind = kind;
    EXPECT_TRUE(
        EvaluateAnimationBackend(request, MakeIOSCapabilities()).CanRun());
  }
}

TEST(AnimationBackendEvaluatorTest,
     IOSRejectsPercentageTransformWithoutCapability) {
  auto keyframes = MakeTransformKeyframes(gfx::LengthUnit::kPercent);
  auto capabilities = MakeIOSCapabilities();
  FindTransformCapability(capabilities)->transform.translate_x_units =
      gfx::kTransformUnitNumber;

  const auto result = EvaluateAnimationBackend(
      MakeRequest(keyframes, gfx::AnimationPropertyType::kTransform),
      capabilities);

  EXPECT_EQ(result.fallback_reason, AnimationFallbackReason::kUnsupportedValue);
}

TEST(AnimationBackendEvaluatorTest, IOSRejectsUnsupportedTransformOperation) {
  auto keyframes = MakeTransformKeyframes();
  auto capabilities = MakeIOSCapabilities();
  auto* transform = FindTransformCapability(capabilities);
  ASSERT_NE(transform, nullptr);
  transform->transform.translate_x_units = 0;
  transform->transform.translate_y_units = 0;
  transform->transform.translate_z_units = 0;

  const auto result = EvaluateAnimationBackend(
      MakeRequest(keyframes, gfx::AnimationPropertyType::kTransform),
      capabilities);

  EXPECT_EQ(result.fallback_reason, AnimationFallbackReason::kUnsupportedValue);
}

TEST(AnimationBackendEvaluatorTest, IOSChecksTranslateAxisCapability) {
  gfx::TransformOperations start_operations;
  start_operations.AppendTranslate({}, {}, {});
  gfx::TransformOperations end_operations;
  end_operations.AppendTranslate({}, {}, {100.0f, gfx::LengthUnit::kNumber});
  auto keyframes = MakeTransformKeyframes(std::move(start_operations),
                                          std::move(end_operations));
  auto capabilities = MakeIOSCapabilities();
  auto* transform = FindTransformCapability(capabilities);
  ASSERT_NE(transform, nullptr);
  transform->transform.translate_z_units = 0;

  const auto result = EvaluateAnimationBackend(
      MakeRequest(keyframes, gfx::AnimationPropertyType::kTransform),
      capabilities);

  EXPECT_EQ(result.fallback_reason, AnimationFallbackReason::kUnsupportedValue);
}

TEST(AnimationBackendEvaluatorTest, IOSChecksTranslateUnitsPerAxis) {
  for (const auto kind :
       {gfx::AnimationKind::kKeyframe, gfx::AnimationKind::kTransition}) {
    for (int axis = 0; axis < 3; ++axis) {
      for (float value : {0.0f, 50.0f}) {
        gfx::LengthValue components[3];
        components[axis] = {value, gfx::LengthUnit::kPercent};
        gfx::TransformOperations start;
        start.AppendTranslate({}, {}, {});
        gfx::TransformOperations end;
        end.AppendTranslate(components[0], components[1], components[2]);
        auto keyframes =
            MakeTransformKeyframes(std::move(start), std::move(end));
        auto request =
            MakeRequest(keyframes, gfx::AnimationPropertyType::kTransform);
        request.kind = kind;
        const auto result =
            EvaluateAnimationBackend(request, MakeIOSCapabilities());
        EXPECT_EQ(result.CanRun(), axis != 2)
            << "axis=" << axis << " value=" << value;
        if (axis == 2) {
          EXPECT_EQ(result.fallback_reason,
                    AnimationFallbackReason::kUnsupportedValue);
        }
      }
    }
  }
}

TEST(AnimationBackendEvaluatorTest, IOSChecksTransformAxes) {
  struct {
    const char* name;
    gfx::TransformOperations start;
    gfx::TransformOperations end;
  } cases[] = {{"rotate-x", {}, {}}, {"scale-y", {}, {}}, {"skew-y", {}, {}}};
  cases[0].start.AppendRotate(gfx::TransformOperation::kRotateX, 0.0f);
  cases[0].end.AppendRotate(gfx::TransformOperation::kRotateX, 90.0f);
  cases[1].start.AppendScale(1.0f, 1.0f);
  cases[1].end.AppendScale(1.0f, 2.0f);
  cases[2].start.AppendSkew(0.0f, 0.0f);
  cases[2].end.AppendSkew(0.0f, 20.0f);

  for (auto& test : cases) {
    SCOPED_TRACE(test.name);
    auto keyframes =
        MakeTransformKeyframes(std::move(test.start), std::move(test.end));
    auto request =
        MakeRequest(keyframes, gfx::AnimationPropertyType::kTransform);
    auto capabilities = MakeIOSCapabilities();
    EXPECT_TRUE(EvaluateAnimationBackend(request, capabilities).CanRun());
    auto* transform = FindTransformCapability(capabilities);
    ASSERT_NE(transform, nullptr);
    transform->transform.rotate_axes = gfx::kTransformAxisZ;
    transform->transform.scale_axes = gfx::kTransformAxisX;
    transform->transform.skew_axes = gfx::kTransformAxisX;

    EXPECT_EQ(EvaluateAnimationBackend(request, capabilities).fallback_reason,
              AnimationFallbackReason::kUnsupportedValue);
  }
}

TEST(AnimationBackendEvaluatorTest, IOSChecksMatrixDimensionCapability) {
  std::array<float, 16> identity = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 0.0f, 1.0f};
  auto translated = identity;
  translated[12] = 100.0f;
  gfx::TransformOperations start_operations;
  start_operations.AppendMatrix(gfx::TransformOperation::kMatrix3d, identity);
  gfx::TransformOperations end_operations;
  end_operations.AppendMatrix(gfx::TransformOperation::kMatrix3d, translated);
  auto keyframes = MakeTransformKeyframes(std::move(start_operations),
                                          std::move(end_operations));
  auto capabilities = MakeIOSCapabilities();
  auto* transform = FindTransformCapability(capabilities);
  ASSERT_NE(transform, nullptr);
  transform->transform.matrix_dimensions = gfx::kTransformMatrix2D;

  const auto result = EvaluateAnimationBackend(
      MakeRequest(keyframes, gfx::AnimationPropertyType::kTransform),
      capabilities);

  EXPECT_EQ(result.fallback_reason, AnimationFallbackReason::kUnsupportedValue);
}

TEST(AnimationBackendEvaluatorTest, IOSRejectsStepsTiming) {
  auto keyframes = MakeOpacityKeyframes(
      gfx::StepsTimingFunction::Create(2, gfx::StepsType::kEnd));

  const auto result =
      EvaluateAnimationBackend(MakeRequest(keyframes), MakeIOSCapabilities());

  EXPECT_EQ(result.fallback_reason,
            AnimationFallbackReason::kUnsupportedTimingFunction);
}

TEST(AnimationBackendEvaluatorTest, IOSPreservesSingleIntervalTiming) {
  auto keyframes = MakeOpacityKeyframes(
      gfx::CubicBezierTimingFunction::Create(0.25, 0.1, 0.25, 1.0));
  const auto result =
      EvaluateAnimationBackend(MakeRequest(keyframes), MakeIOSCapabilities());
  ASSERT_TRUE(result.CanRun());
  auto actual = gfx::CreateTimingFunction(result.platform_timing);
  EXPECT_NEAR(actual->GetValue(0.25),
              keyframes.front()->timing_function()->GetValue(0.25), 1e-6);

  auto capabilities = MakeIOSCapabilities();
  capabilities.properties.front().timing_functions = gfx::kTimingFunctionLinear;
  EXPECT_EQ(EvaluateAnimationBackend(MakeRequest(keyframes), capabilities)
                .fallback_reason,
            AnimationFallbackReason::kUnsupportedTimingFunction);
}

TEST(AnimationBackendEvaluatorTest, IOSRoutesExplicitIdentityKeyframeTiming) {
  auto capabilities = MakeIOSCapabilities();
  // The backend needs no cubic-bezier support for a linear-equivalent curve.
  capabilities.properties.front().timing_functions = gfx::kTimingFunctionLinear;
  for (bool cubic : {false, true}) {
    std::vector<std::unique_ptr<gfx::Keyframe>> keyframes;
    for (double offset : {0.0, 0.5, 1.0}) {
      std::unique_ptr<gfx::TimingFunction> timing =
          cubic ? std::unique_ptr<gfx::TimingFunction>(
                      gfx::CubicBezierTimingFunction::Create(0.25, 0.25, 0.75,
                                                             0.75))
                : gfx::LinearTimingFunction::Create();
      auto frame = gfx::FloatKeyframe::Create(
          fml::TimeDelta::FromSecondsF(offset), std::move(timing));
      frame->SetValue(offset);
      keyframes.push_back(std::move(frame));
    }
    EXPECT_TRUE(EvaluateAnimationBackend(MakeRequest(keyframes), capabilities)
                    .CanRun());
    keyframes.erase(keyframes.begin() + 1);
    const auto result =
        EvaluateAnimationBackend(MakeRequest(keyframes), capabilities);
    EXPECT_TRUE(result.CanRun());
    EXPECT_EQ(result.platform_timing.timing_func,
              gfx::TimingFunctionType::kLinear);
  }
}

TEST(AnimationBackendEvaluatorTest, IOSIgnoresUnusedLastKeyframeTiming) {
  auto keyframes = MakeOpacityKeyframes();
  auto end = gfx::FloatKeyframe::Create(
      fml::TimeDelta::FromSecondsF(1.0),
      gfx::StepsTimingFunction::Create(2, gfx::StepsType::kEnd));
  end->SetValue(1.0f);
  keyframes.back() = std::move(end);
  EXPECT_TRUE(
      EvaluateAnimationBackend(MakeRequest(keyframes), MakeIOSCapabilities())
          .CanRun());
}

TEST(AnimationBackendEvaluatorTest,
     IdenticalNonlinearSegmentTimingStillFallsBack) {
  std::vector<std::unique_ptr<gfx::Keyframe>> keyframes;
  for (double offset : {0.0, 0.5, 1.0}) {
    auto frame = gfx::FloatKeyframe::Create(
        fml::TimeDelta::FromSecondsF(offset),
        gfx::CubicBezierTimingFunction::CreatePreset(
            gfx::CubicBezierTimingFunction::EaseType::EASE_IN));
    frame->SetValue(offset);
    keyframes.push_back(std::move(frame));
  }
  EXPECT_EQ(
      EvaluateAnimationBackend(MakeRequest(keyframes), MakeIOSCapabilities())
          .fallback_reason,
      AnimationFallbackReason::kUnsupportedTimingFunction);
}

TEST(AnimationBackendEvaluatorTest, IOSRejectsNonlinearTimingOnLaterSegment) {
  auto keyframes = MakeOpacityKeyframes();
  auto middle = gfx::FloatKeyframe::Create(
      fml::TimeDelta::FromSecondsF(0.5),
      gfx::CubicBezierTimingFunction::CreatePreset(
          gfx::CubicBezierTimingFunction::EaseType::EASE_IN));
  middle->SetValue(0.5f);
  keyframes.insert(keyframes.begin() + 1, std::move(middle));
  EXPECT_EQ(
      EvaluateAnimationBackend(MakeRequest(keyframes), MakeIOSCapabilities())
          .fallback_reason,
      AnimationFallbackReason::kUnsupportedTimingFunction);
}

TEST(AnimationBackendEvaluatorTest, IOSRoutesOpacityTransition) {
  auto keyframes = MakeOpacityKeyframes();
  auto request = MakeRequest(keyframes);
  request.kind = gfx::AnimationKind::kTransition;

  const auto result = EvaluateAnimationBackend(request, MakeIOSCapabilities());

  EXPECT_TRUE(result.CanRun());
}

TEST(AnimationBackendEvaluatorTest, LayoutAlwaysStaysInCore) {
  auto keyframes = MakeOpacityKeyframes();
  auto request = MakeRequest(keyframes);
  request.property = gfx::AnimationPropertyType::kLeft;

  const auto result = EvaluateAnimationBackend(request, MakeIOSCapabilities());

  EXPECT_EQ(result.fallback_reason,
            AnimationFallbackReason::kRequiresCoreLayout);
}

TEST(AnimationBackendEvaluatorTest, FlexGrowAlwaysStaysInCore) {
  auto keyframes = MakeOpacityKeyframes();
  auto capabilities = MakeIOSCapabilities();
  auto flex_grow = capabilities.properties.front();
  flex_grow.property = gfx::AnimationPropertyType::kFlexGrow;
  capabilities.properties.push_back(flex_grow);
  auto request = MakeRequest(keyframes);
  request.property = gfx::AnimationPropertyType::kFlexGrow;

  const auto result = EvaluateAnimationBackend(request, capabilities);

  EXPECT_EQ(result.fallback_reason,
            AnimationFallbackReason::kRequiresCoreLayout);
}

TEST(AnimationBackendEvaluatorTest, DynamicDependencyStaysInCore) {
  auto keyframes = MakeOpacityKeyframes();
  auto request = MakeRequest(keyframes);
  request.has_dynamic_dependencies = true;

  const auto result = EvaluateAnimationBackend(request, MakeIOSCapabilities());

  EXPECT_EQ(result.fallback_reason,
            AnimationFallbackReason::kDynamicDependency);
}

TEST(AnimationBackendEvaluatorTest, ValueValidationIsNotRoutingPolicy) {
  auto keyframes = MakeOpacityKeyframes();
  static_cast<gfx::FloatKeyframe*>(keyframes.back().get())->SetValue(1.1f);

  const auto result =
      EvaluateAnimationBackend(MakeRequest(keyframes), MakeIOSCapabilities());

  EXPECT_TRUE(result.CanRun());
}

TEST(AnimationBackendEvaluatorTest, DisabledBackendStaysInCore) {
  auto keyframes = MakeOpacityKeyframes();
  auto capabilities = MakeIOSCapabilities();
  capabilities.backend = gfx::AnimationBackendType::kNone;

  const auto result =
      EvaluateAnimationBackend(MakeRequest(keyframes), capabilities);

  EXPECT_EQ(result.fallback_reason,
            AnimationFallbackReason::kBackendUnavailable);
}

}  // namespace
}  // namespace animation
}  // namespace lynx
