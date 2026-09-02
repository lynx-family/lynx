// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/animation_backend_evaluator.h"

#include <algorithm>

#include "gfx/animation/timing_function.h"

namespace lynx {
namespace animation {
namespace {

bool PropertyAffectsLayout(gfx::AnimationPropertyType property) {
  using Property = gfx::AnimationPropertyType;
  switch (property) {
    case Property::kWidth:
    case Property::kHeight:
    case Property::kLeft:
    case Property::kTop:
    case Property::kRight:
    case Property::kBottom:
    case Property::kMaxWidth:
    case Property::kMinWidth:
    case Property::kMaxHeight:
    case Property::kMinHeight:
    case Property::kPaddingLeft:
    case Property::kPaddingRight:
    case Property::kPaddingTop:
    case Property::kPaddingBottom:
    case Property::kMarginLeft:
    case Property::kMarginRight:
    case Property::kMarginTop:
    case Property::kMarginBottom:
    case Property::kBorderLeftWidth:
    case Property::kBorderRightWidth:
    case Property::kBorderTopWidth:
    case Property::kBorderBottomWidth:
    case Property::kFlexBasis:
    case Property::kFlexGrow:
      return true;
    default:
      return false;
  }
}

bool IsTimingFunctionSupported(
    const gfx::TimingFunction* timing_function,
    gfx::TimingFunctionCapabilityMask supported_types) {
  if (timing_function == nullptr ||
      timing_function->GetType() == gfx::TimingFunction::Type::LINEAR) {
    return (supported_types & gfx::kTimingFunctionLinear) != 0;
  }
  if (timing_function->GetType() == gfx::TimingFunction::Type::CUBIC_BEZIER) {
    return (supported_types & gfx::kTimingFunctionCubicBezier) != 0;
  }
  if (timing_function->GetType() == gfx::TimingFunction::Type::STEPS) {
    return (supported_types & gfx::kTimingFunctionSteps) != 0;
  }
  return false;
}

AnimationBackendResult Unsupported(AnimationFallbackReason reason) {
  return {false, reason};
}

bool SupportsTransformAxes(gfx::TransformAxisCapabilityMask supported,
                           gfx::TransformAxisCapabilityMask required) {
  return supported != 0 && (required & ~supported) == 0;
}

bool UsesTranslateComponent(const gfx::LengthValue& value) {
  return value.value != 0.0f || value.unit != gfx::LengthUnit::kNumber;
}

gfx::TransformAxisCapabilityMask RequiredTranslateAxes(
    const gfx::TransformOperation& operation) {
  gfx::TransformAxisCapabilityMask axes = 0;
  if (UsesTranslateComponent(operation.translate.x)) {
    axes |= gfx::kTransformAxisX;
  }
  if (UsesTranslateComponent(operation.translate.y)) {
    axes |= gfx::kTransformAxisY;
  }
  if (UsesTranslateComponent(operation.translate.z)) {
    axes |= gfx::kTransformAxisZ;
  }
  return axes;
}

gfx::TransformUnitCapabilityMask RequiredTranslateUnits(
    const gfx::TransformOperation& operation) {
  gfx::TransformUnitCapabilityMask units = 0;
  for (const auto& value :
       {operation.translate.x, operation.translate.y, operation.translate.z}) {
    if (!UsesTranslateComponent(value)) {
      continue;
    }
    units |= value.unit == gfx::LengthUnit::kPercent
                 ? gfx::kTransformUnitPercent
                 : gfx::kTransformUnitNumber;
  }
  return units;
}

gfx::TransformAxisCapabilityMask RequiredXYAxes(float x, float x_neutral,
                                                float y, float y_neutral) {
  gfx::TransformAxisCapabilityMask axes = 0;
  if (x != x_neutral) {
    axes |= gfx::kTransformAxisX;
  }
  if (y != y_neutral) {
    axes |= gfx::kTransformAxisY;
  }
  return axes;
}

bool IsTransformOperationSupported(
    const gfx::TransformOperation& operation,
    const gfx::AnimationPropertyCapability& capability) {
  if (operation.type == gfx::TransformOperation::kIdentity) {
    return true;
  }
  switch (operation.type) {
    case gfx::TransformOperation::kTranslate: {
      const auto axes = RequiredTranslateAxes(operation);
      const auto units = RequiredTranslateUnits(operation);
      return SupportsTransformAxes(capability.transform.translate_axes, axes) &&
             (units & ~capability.transform.translate_units) == 0;
    }
    case gfx::TransformOperation::kRotateX:
      return SupportsTransformAxes(capability.transform.rotate_axes,
                                   gfx::kTransformAxisX);
    case gfx::TransformOperation::kRotateY:
      return SupportsTransformAxes(capability.transform.rotate_axes,
                                   gfx::kTransformAxisY);
    case gfx::TransformOperation::kRotateZ:
      return SupportsTransformAxes(capability.transform.rotate_axes,
                                   gfx::kTransformAxisZ);
    case gfx::TransformOperation::kScale:
      return SupportsTransformAxes(
          capability.transform.scale_axes,
          RequiredXYAxes(operation.scale.x, 1.0f, operation.scale.y, 1.0f));
    case gfx::TransformOperation::kSkew:
      return SupportsTransformAxes(
          capability.transform.skew_axes,
          RequiredXYAxes(operation.skew.x, 0.0f, operation.skew.y, 0.0f));
    case gfx::TransformOperation::kMatrix:
      return (capability.transform.matrix_dimensions &
              gfx::kTransformMatrix2D) != 0;
    case gfx::TransformOperation::kMatrix3d:
      return (capability.transform.matrix_dimensions &
              gfx::kTransformMatrix3D) != 0;
    default:
      return false;
  }
}

bool IsTransformValueSupported(
    const gfx::Keyframe* keyframe,
    const gfx::AnimationPropertyCapability& capability) {
  const auto* transform_keyframe =
      static_cast<const gfx::TransformKeyframe*>(keyframe);
  if (!transform_keyframe->HasResolvedValue()) {
    return false;
  }
  const auto& operations = transform_keyframe->ResolvedValue();
  return std::all_of(
      operations.GetOperations().begin(), operations.GetOperations().end(),
      [&capability](const gfx::TransformOperation& operation) {
        return IsTransformOperationSupported(operation, capability);
      });
}

}  // namespace

AnimationBackendResult EvaluateAnimationBackend(
    const AnimationBackendRequest& request,
    const gfx::AnimationBackendCapabilities& capabilities) {
  if (PropertyAffectsLayout(request.property)) {
    return Unsupported(AnimationFallbackReason::kRequiresCoreLayout);
  }
  if (capabilities.backend == gfx::AnimationBackendType::kNone) {
    return Unsupported(AnimationFallbackReason::kBackendUnavailable);
  }
  if (request.has_dynamic_dependencies) {
    return Unsupported(AnimationFallbackReason::kDynamicDependency);
  }
  const auto value_type = request.keyframes.front()->ValueType();
  const auto* property_capability =
      capabilities.FindProperty(request.kind, request.property, value_type);
  if (property_capability == nullptr) {
    return Unsupported(AnimationFallbackReason::kUnsupportedProperty);
  }

  for (const auto* keyframe : request.keyframes) {
    if (value_type == gfx::KeyframeValueType::kTransform &&
        !IsTransformValueSupported(keyframe, *property_capability)) {
      return Unsupported(AnimationFallbackReason::kUnsupportedValue);
    }
    if (keyframe->timing_function() != nullptr) {
      if (!property_capability->supports_per_keyframe_timing ||
          !IsTimingFunctionSupported(keyframe->timing_function(),
                                     property_capability->timing_functions)) {
        return Unsupported(AnimationFallbackReason::kUnsupportedTimingFunction);
      }
    }
  }

  auto curve_timing =
      gfx::CreateTimingFunction(request.animation_data->timing_func);
  if (curve_timing == nullptr ||
      !IsTimingFunctionSupported(curve_timing.get(),
                                 property_capability->timing_functions)) {
    return Unsupported(AnimationFallbackReason::kUnsupportedTimingFunction);
  }

  return {true, AnimationFallbackReason::kNone};
}

}  // namespace animation
}  // namespace lynx
