// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/animation_backend_evaluator.h"

#include <algorithm>
#include <memory>

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

bool IsIdentityTimingFunction(const gfx::TimingFunction* timing_function) {
  if (timing_function == nullptr ||
      timing_function->GetType() == gfx::TimingFunction::Type::LINEAR) {
    return true;
  }
  if (timing_function->GetType() == gfx::TimingFunction::Type::CUBIC_BEZIER) {
    const auto& bezier =
        static_cast<const gfx::CubicBezierTimingFunction*>(timing_function)
            ->bezier();
    return bezier.GetX1() == bezier.GetY1() && bezier.GetX2() == bezier.GetY2();
  }
  return false;
}

gfx::TimingFunctionData ToPlatformTiming(const gfx::TimingFunction* timing) {
  gfx::TimingFunctionData result;
  if (IsIdentityTimingFunction(timing)) {
    return result;
  }
  if (timing->GetType() == gfx::TimingFunction::Type::CUBIC_BEZIER) {
    const auto& bezier =
        static_cast<const gfx::CubicBezierTimingFunction*>(timing)->bezier();
    result.timing_func = gfx::TimingFunctionType::kCubicBezier;
    result.x1 = bezier.GetX1();
    result.y1 = bezier.GetY1();
    result.x2 = bezier.GetX2();
    result.y2 = bezier.GetY2();
  } else if (timing->GetType() == gfx::TimingFunction::Type::STEPS) {
    const auto* steps = static_cast<const gfx::StepsTimingFunction*>(timing);
    result.timing_func = gfx::TimingFunctionType::kSteps;
    result.x1 = steps->steps();
    result.steps_type = steps->step_position();
  }
  return result;
}

AnimationBackendResult Unsupported(AnimationFallbackReason reason) {
  return {false, reason};
}

// `required` describes the axes needed by the current keyframe's transform
// operation; `supported` comes from the backend capability table. Even when all
// components are neutral, the backend must support the operation on some axis.
bool SupportsTransformAxes(gfx::TransformAxisCapabilityMask supported,
                           gfx::TransformAxisCapabilityMask required) {
  return supported != 0 && (required & ~supported) == 0;
}

// A translate component of {0, kNumber} is the default for an unused axis and
// needs no axis/unit support. A percentage, including 0%, still requires
// percent support because the typed value sent to the backend retains that
// unit.
bool RequiresTranslateComponentSupport(const gfx::LengthValue& value) {
  return value.value != 0.0f || value.unit != gfx::LengthUnit::kNumber;
}

bool IsTranslateComponentSupported(
    const gfx::LengthValue& value,
    gfx::TransformUnitCapabilityMask supported_units) {
  if (!RequiresTranslateComponentSupport(value)) {
    return true;
  }
  // Match this keyframe component's unit against the backend's units for its
  // axis.
  const auto required_unit = value.unit == gfx::LengthUnit::kPercent
                                 ? gfx::kTransformUnitPercent
                                 : gfx::kTransformUnitNumber;
  return (supported_units & required_unit) != 0;
}

// Collect the axes needed by this keyframe's scale or skew operation. Neutral
// components (1 for scale, 0 for skew) have no effect and are skipped. For
// example, scale(1, 2) requires Y-axis scale support only.
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
      const auto& transform = capability.transform;
      return (transform.translate_x_units | transform.translate_y_units |
              transform.translate_z_units) != 0 &&
             IsTranslateComponentSupported(operation.translate.x,
                                           transform.translate_x_units) &&
             IsTranslateComponentSupported(operation.translate.y,
                                           transform.translate_y_units) &&
             IsTranslateComponentSupported(operation.translate.z,
                                           transform.translate_z_units);
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
    // TODO: Replace this blanket fallback with dependency-specific backend
    // checks:
    // - var(): A resolved value may be representable by the backend. Track
    // custom
    //   property changes and update the platform effect when the value changes;
    //   resolving the variable once does not make the dependency stable.
    // - em/rem/sp: Resolve using the relevant font context and propagate
    // font-size
    //   or font-scale changes to the platform effect.
    // - vw/vh/rpx: Resolve using the relevant viewport or unit-conversion
    // context
    //   and propagate changes to that context.
    // - Fixed-unit calc(), e.g. calc(10px + 20px): Fold to a numeric value when
    // no
    //   dynamic inputs remain. The current dependency collector conservatively
    //   marks retained calc values as depending on all supported dynamic units.
    // - Context-dependent calc(), e.g. calc(1em + 10px) or calc(50% + 10px):
    // Track
    //   the actual font, viewport, or element-size inputs. Preserve an
    //   expression the backend can evaluate, or re-resolve and update it when
    //   inputs change.
    // - Pure percentage transforms, e.g. translateX(50%): Already retain a
    // typed
    //   percentage and bypass this flag. Keep checking axis-specific backend
    //   capabilities and ensure reference-size changes are handled correctly.
    // Allow routing only when the platform path can represent the resolved
    // value and handle dependency updates while preserving animation progress
    // and lifecycle semantics. Fall back to New Animator for dependencies the
    // platform cannot track.
    return Unsupported(AnimationFallbackReason::kDynamicDependency);
  }
  const auto value_type = request.keyframes.front()->ValueType();
  const auto* property_capability =
      capabilities.FindProperty(request.kind, request.property, value_type);
  if (property_capability == nullptr) {
    return Unsupported(AnimationFallbackReason::kUnsupportedProperty);
  }

  std::unique_ptr<gfx::TimingFunction> default_timing;
  AnimationBackendResult result{true, AnimationFallbackReason::kNone};
  const bool single_interval = request.keyframes.size() == 2;
  for (size_t index = 0; index < request.keyframes.size(); ++index) {
    const auto* keyframe = request.keyframes[index];
    if (value_type == gfx::KeyframeValueType::kTransform &&
        !IsTransformValueSupported(keyframe, *property_capability)) {
      return Unsupported(AnimationFallbackReason::kUnsupportedValue);
    }
    // CSS keyframe timing overrides the animation default for the outgoing
    // interval. The last frame has no outgoing interval. Do not separately
    // reject an animation default that every active interval overrides.
    if (index + 1 == request.keyframes.size()) {
      continue;
    }
    const auto* timing = keyframe->timing_function();
    if (timing == nullptr) {
      if (default_timing == nullptr) {
        default_timing =
            gfx::CreateTimingFunction(request.animation_data->timing_func);
      }
      timing = default_timing.get();
    }
    if (!IsTimingFunctionSupported(timing,
                                   property_capability->timing_functions)) {
      return Unsupported(AnimationFallbackReason::kUnsupportedTimingFunction);
    }
    if (single_interval) {
      // A single interval can use the platform's effect-wide timing, including
      // an explicit keyframe override. The adapter receives this resolved
      // value.
      result.platform_timing = ToPlatformTiming(timing);
    } else if (!IsIdentityTimingFunction(timing) &&
               !property_capability->supports_per_keyframe_timing) {
      // Equal non-linear functions still restart at every CSS interval. A
      // single effect-wide function cannot generally reproduce those
      // boundaries.
      // TODO: Enable this only when the backend preserves interval timing,
      // including synthesized transform rotation frames and reverse playback.
      return Unsupported(AnimationFallbackReason::kUnsupportedTimingFunction);
    }
  }
  return result;
}

}  // namespace animation
}  // namespace lynx
