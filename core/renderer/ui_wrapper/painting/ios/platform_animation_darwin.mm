// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_animation_darwin.h"

#import <Lynx/LynxAnimationInfo.h>
#import <Lynx/LynxConverter+Transform.h>
#import <Lynx/LynxKeyframeAnimator.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxPlatformAnimation.h>
#import <Lynx/LynxTransformRaw.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>

namespace lynx::tasm {
namespace {
CAMediaTimingFunction* ToCAMediaTimingFunction(const gfx::TimingFunctionData& timing) {
  using Type = gfx::TimingFunctionType;
  switch (timing.timing_func) {
    case Type::kLinear:
      return [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
    case Type::kEaseIn:
      return [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
    case Type::kEaseOut:
      return [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
    case Type::kEaseInEaseOut:
      return [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
    case Type::kSquareBezier:
      return [CAMediaTimingFunction functionWithControlPoints:timing.x1:timing.y1:1.0:1.0];
    case Type::kCubicBezier:
      return
          [CAMediaTimingFunction functionWithControlPoints:timing.x1:timing.y1:timing.x2:timing.y2];
    case Type::kSteps:
      return nil;
  }
  return nil;
}

CAMediaTimingFillMode ToCAMediaTimingFillMode(gfx::AnimationFillModeType fill_mode) {
  switch (fill_mode) {
    case gfx::AnimationFillModeType::kForwards:
      return kCAFillModeForwards;
    case gfx::AnimationFillModeType::kBackwards:
      return kCAFillModeBackwards;
    case gfx::AnimationFillModeType::kBoth:
      return kCAFillModeBoth;
    case gfx::AnimationFillModeType::kNone:
      return kCAFillModeRemoved;
  }
  return kCAFillModeRemoved;
}

LynxAnimationInfo* MakeAnimationInfo(const gfx::PlatformAnimationCommand& command) {
  NSString* name = [NSString stringWithUTF8String:command.name.c_str()];
  if (name == nil) {
    return nil;
  }
  const auto& data = command.animation_data;
  CAMediaTimingFunction* timing_function = ToCAMediaTimingFunction(data.timing_func);
  if (timing_function == nil) {
    return nil;
  }
  LynxAnimationInfo* info = [[LynxAnimationInfo alloc] initWithName:name];
  info.duration = static_cast<NSTimeInterval>(data.duration) / 1000.0;
  info.delay = static_cast<NSTimeInterval>(data.delay) / 1000.0;
  info.timingFunction = timing_function;
  info.iterationCount = data.iteration_count;
  info.direction = static_cast<LynxAnimationDirectionType>(data.direction);
  info.fillMode = ToCAMediaTimingFillMode(data.fill_mode);
  info.playState = static_cast<LynxAnimationPlayStateType>(data.play_state);
  if (command.kind == gfx::AnimationKind::kTransition) {
    if (command.properties.size() != 1) {
      return nil;
    }
    switch (command.properties.front().property) {
      case gfx::AnimationPropertyType::kOpacity:
        info.prop = OPACITY;
        break;
      case gfx::AnimationPropertyType::kTransform:
        info.prop = TRANSITION_TRANSFORM;
        break;
      default:
        return nil;
    }
    info.name = [LynxConverter toLynxPropName:info.prop];
  }
  return info;
}

bool GetOpacityTransitionEndpoints(const gfx::PlatformAnimationCommand& command,
                                   CGFloat* from_opacity, CGFloat* to_opacity) {
  if (command.properties.size() != 1 ||
      command.properties.front().property != gfx::AnimationPropertyType::kOpacity) {
    return false;
  }
  const auto& keyframes = command.properties.front().keyframes;
  if (keyframes.size() < 2 || keyframes.front() == nullptr || keyframes.back() == nullptr ||
      keyframes.front()->IsEmpty() || keyframes.back()->IsEmpty() ||
      keyframes.front()->ValueType() != gfx::KeyframeValueType::kFloat ||
      keyframes.back()->ValueType() != gfx::KeyframeValueType::kFloat) {
    return false;
  }
  *from_opacity = static_cast<const gfx::FloatKeyframe*>(keyframes.front().get())->Value();
  *to_opacity = static_cast<const gfx::FloatKeyframe*>(keyframes.back().get())->Value();
  return true;
}

NSArray<LynxTransformRaw*>* ToLynxTransformRaw(const gfx::TransformOperations& operations);

bool GetTransformTransitionEndpoints(const gfx::PlatformAnimationCommand& command,
                                     NSArray<LynxTransformRaw*>** from_transform,
                                     NSArray<LynxTransformRaw*>** to_transform) {
  if (command.properties.size() != 1 ||
      command.properties.front().property != gfx::AnimationPropertyType::kTransform) {
    return false;
  }
  const auto& keyframes = command.properties.front().keyframes;
  if (keyframes.size() < 2 || keyframes.front() == nullptr || keyframes.back() == nullptr ||
      keyframes.front()->IsEmpty() || keyframes.back()->IsEmpty() ||
      keyframes.front()->ValueType() != gfx::KeyframeValueType::kTransform ||
      keyframes.back()->ValueType() != gfx::KeyframeValueType::kTransform) {
    return false;
  }
  const auto* from_keyframe = static_cast<const gfx::TransformKeyframe*>(keyframes.front().get());
  const auto* to_keyframe = static_cast<const gfx::TransformKeyframe*>(keyframes.back().get());
  if (!from_keyframe->HasResolvedValue() || !to_keyframe->HasResolvedValue()) {
    return false;
  }
  *from_transform = ToLynxTransformRaw(from_keyframe->ResolvedValue());
  *to_transform = ToLynxTransformRaw(to_keyframe->ResolvedValue());
  return true;
}

LynxKeyframeParsedData* MakeOpacityKeyframes(const gfx::PlatformAnimationProperty& property,
                                             const gfx::AnimationData& animation_data) {
  if (property.property != gfx::AnimationPropertyType::kOpacity || property.keyframes.size() < 2) {
    return nil;
  }

  LynxKeyframeParsedData* parsed_data = [[LynxKeyframeParsedData alloc] init];
  NSMutableArray* values = [[NSMutableArray alloc] init];
  NSMutableArray<NSNumber*>* times = [[NSMutableArray alloc] init];
  const bool reverse = animation_data.direction == gfx::AnimationDirectionType::kReverse ||
                       animation_data.direction == gfx::AnimationDirectionType::kAlternateReverse;

  auto append_keyframe = [&](const std::shared_ptr<const gfx::Keyframe>& keyframe) {
    if (keyframe == nullptr || keyframe->IsEmpty() ||
        keyframe->ValueType() != gfx::KeyframeValueType::kFloat) {
      return false;
    }
    const auto* float_keyframe = static_cast<const gfx::FloatKeyframe*>(keyframe.get());
    const double offset = reverse ? 1.0 - keyframe->Offset() : keyframe->Offset();
    NSNumber* value = @(float_keyframe->Value());
    [values addObject:value];
    [times addObject:@(offset)];
    if (keyframe->Offset() == 0.0) {
      parsed_data.beginStyles[@"opacity"] = value;
    } else if (keyframe->Offset() == 1.0) {
      parsed_data.endStyles[@"opacity"] = value;
    }
    return true;
  };

  if (reverse) {
    for (auto iter = property.keyframes.rbegin(); iter != property.keyframes.rend(); ++iter) {
      if (!append_keyframe(*iter)) {
        return nil;
      }
    }
  } else {
    for (const auto& keyframe : property.keyframes) {
      if (!append_keyframe(keyframe)) {
        return nil;
      }
    }
  }
  parsed_data.keyframeValues[@"opacity"] = values;
  parsed_data.keyframeTimes[@"opacity"] = times;
  return parsed_data;
}

LynxPlatformLengthUnit ToLynxLengthUnit(gfx::LengthUnit unit) {
  return unit == gfx::LengthUnit::kPercent ? LynxPlatformLengthUnitPercentage
                                           : LynxPlatformLengthUnitNumber;
}

NSNumber* ToLynxLengthValue(const gfx::LengthValue& value) {
  // gfx stores CSS percentages as percentage points. LynxPlatformLength uses
  // fractions for percentage transforms.
  return @(value.unit == gfx::LengthUnit::kPercent ? value.value / 100.0f : value.value);
}

NSArray<LynxTransformRaw*>* ToLynxTransformRaw(const gfx::TransformOperations& operations) {
  NSMutableArray<LynxTransformRaw*>* result =
      [[NSMutableArray alloc] initWithCapacity:operations.GetOperations().size()];
  for (const auto& operation : operations.GetOperations()) {
    if (operation.type == gfx::TransformOperation::kIdentity) {
      continue;
    }
    LynxTransformRaw* raw;
    if (operation.type == gfx::TransformOperation::kTranslate) {
      auto length = [](const gfx::LengthValue& value) {
        return [[LynxPlatformLength alloc] initWithValue:ToLynxLengthValue(value)
                                                    type:ToLynxLengthUnit(value.unit)];
      };
      raw = [[LynxTransformRaw alloc] initWithTranslationX:length(operation.translate.x)
                                                         y:length(operation.translate.y)
                                                         z:length(operation.translate.z)];
    } else {
      raw = [[LynxTransformRaw alloc] init];
      switch (operation.type) {
        case gfx::TransformOperation::kRotateX:
        case gfx::TransformOperation::kRotateY:
        case gfx::TransformOperation::kRotateZ:
          raw.type = operation.type == gfx::TransformOperation::kRotateX ? LynxTransformTypeRotateX
                     : operation.type == gfx::TransformOperation::kRotateY
                         ? LynxTransformTypeRotateY
                         : LynxTransformTypeRotateZ;
          raw.p0 = operation.rotate.degree;
          break;
        case gfx::TransformOperation::kScale:
          raw.type = LynxTransformTypeScale;
          raw.p0 = operation.scale.x;
          raw.p1 = operation.scale.y;
          break;
        case gfx::TransformOperation::kSkew:
          raw.type = LynxTransformTypeSkew;
          raw.p0 = operation.skew.x;
          raw.p1 = operation.skew.y;
          break;
        case gfx::TransformOperation::kMatrix:
        case gfx::TransformOperation::kMatrix3d: {
          raw.type = operation.type == gfx::TransformOperation::kMatrix ? LynxTransformTypeMatrix
                                                                        : LynxTransformTypeMatrix3d;
          const auto& m = operation.matrix.matrix_data;
          raw.transformMatrix =
              (CATransform3D){m[0], m[1], m[2],  m[3],  m[4],  m[5],  m[6],  m[7],
                              m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]};
          break;
        }
        default:
          continue;
      }
    }
    [result addObject:raw];
  }
  return result;
}

LynxKeyframeParsedData* MakeTransformKeyframes(const gfx::PlatformAnimationProperty& property,
                                               const gfx::AnimationData& animation_data,
                                               LynxUI* ui) {
  if (property.property != gfx::AnimationPropertyType::kTransform ||
      property.keyframes.size() < 2 || ui == nil) {
    return nil;
  }
  NSMutableArray<NSArray<LynxTransformRaw*>*>* values = [[NSMutableArray alloc] init];
  NSMutableArray<NSNumber*>* times = [[NSMutableArray alloc] init];
  const bool reverse = animation_data.direction == gfx::AnimationDirectionType::kReverse ||
                       animation_data.direction == gfx::AnimationDirectionType::kAlternateReverse;

  auto append_keyframe = [&](const std::shared_ptr<const gfx::Keyframe>& keyframe) {
    if (keyframe == nullptr || keyframe->IsEmpty() ||
        keyframe->ValueType() != gfx::KeyframeValueType::kTransform) {
      return false;
    }
    const auto* transform_keyframe = static_cast<const gfx::TransformKeyframe*>(keyframe.get());
    if (!transform_keyframe->HasResolvedValue()) {
      return false;
    }
    [values addObject:ToLynxTransformRaw(transform_keyframe->ResolvedValue())];
    [times addObject:@(reverse ? 1.0 - keyframe->Offset() : keyframe->Offset())];
    return true;
  };

  if (reverse) {
    for (auto iter = property.keyframes.rbegin(); iter != property.keyframes.rend(); ++iter) {
      if (!append_keyframe(*iter)) {
        return nil;
      }
    }
  } else {
    for (const auto& keyframe : property.keyframes) {
      if (!append_keyframe(keyframe)) {
        return nil;
      }
    }
  }
  LynxKeyframeParsedData* parsed = [LynxKeyframeAnimator buildParsedTransformKeyframes:values
                                                                                 times:times
                                                                                    ui:ui];
  if (reverse) {
    NSMutableDictionary* beginStyles = parsed.beginStyles;
    parsed.beginStyles = parsed.endStyles;
    parsed.endStyles = beginStyles;
  }
  return parsed;
}

LynxKeyframeParsedData* MakeTypedKeyframes(const gfx::PlatformAnimationProperty& property,
                                           const gfx::AnimationData& animation_data, LynxUI* ui) {
  switch (property.property) {
    case gfx::AnimationPropertyType::kOpacity:
      return MakeOpacityKeyframes(property, animation_data);
    case gfx::AnimationPropertyType::kTransform:
      return MakeTransformKeyframes(property, animation_data, ui);
    default:
      return nil;
  }
}

void MergeParsedKeyframes(LynxKeyframeParsedData* target, LynxKeyframeParsedData* source) {
  [target.keyframeValues addEntriesFromDictionary:source.keyframeValues];
  [target.keyframeTimes addEntriesFromDictionary:source.keyframeTimes];
  [target.beginStyles addEntriesFromDictionary:source.beginStyles];
  [target.endStyles addEntriesFromDictionary:source.endStyles];
  target.isPercentTransform |= source.isPercentTransform;
}

}  // namespace

void ApplyPlatformAnimationCommands(
    LynxUI* ui, const std::shared_ptr<gfx::PlatformAnimationCommandBatch>& commands) {
  if (commands == nullptr || commands->empty()) {
    return;
  }
  if (ui == nil) {
    return;
  }

  const long sign = static_cast<long>(ui.sign);
  for (const auto& command : *commands) {
    if (command.kind == gfx::AnimationKind::kTransition) {
      [ui prepareTransitionAnimationManager];
      const bool cancel = command.type == gfx::PlatformAnimationCommandType::kCancel;
      if (command.properties.size() != 1) {
        LLogWarn(@"[AnimationRouting] executor=ios-legacy-transition-animator "
                  "element_id=%ld name=%s animation_id=%llu generation=%u "
                  "rejected=invalid-property-count",
                 sign, command.name.c_str(), static_cast<unsigned long long>(command.animation_id),
                 command.generation);
        continue;
      }
      LynxAnimationInfo* info = cancel ? nil : MakeAnimationInfo(command);
      const auto property = command.properties.front().property;
      bool valid = cancel || info != nil;
      switch (property) {
        case gfx::AnimationPropertyType::kOpacity: {
          CGFloat from_opacity = 0.0;
          CGFloat to_opacity = 0.0;
          valid = valid &&
                  (cancel || GetOpacityTransitionEndpoints(command, &from_opacity, &to_opacity));
          if (valid) {
            [ui.transitionAnimationManager applyPlatformOpacityTransition:info
                                                              fromOpacity:from_opacity
                                                                toOpacity:to_opacity
                                                              animationID:command.animation_id
                                                               generation:command.generation
                                                                   cancel:cancel];
          }
          break;
        }
        case gfx::AnimationPropertyType::kTransform: {
          NSArray<LynxTransformRaw*>* from_transform = nil;
          NSArray<LynxTransformRaw*>* to_transform = nil;
          valid = valid && (cancel || GetTransformTransitionEndpoints(command, &from_transform,
                                                                      &to_transform));
          if (valid) {
            [ui.transitionAnimationManager applyPlatformTransformTransition:info
                                                           fromTransformRaw:from_transform
                                                             toTransformRaw:to_transform
                                                                animationID:command.animation_id
                                                                 generation:command.generation
                                                                     cancel:cancel];
          }
          break;
        }
        default:
          valid = false;
          break;
      }
      if (!valid) {
        LLogWarn(@"[AnimationRouting] executor=ios-legacy-transition-animator "
                  "element_id=%ld name=%s animation_id=%llu generation=%u "
                  "rejected=invalid-typed-transition",
                 sign, command.name.c_str(), static_cast<unsigned long long>(command.animation_id),
                 command.generation);
        continue;
      }
      continue;
    }
    if (command.kind != gfx::AnimationKind::kKeyframe) {
      continue;
    }
    const bool cancel = command.type == gfx::PlatformAnimationCommandType::kCancel;
    if (cancel) {
      [ui.animationManager applyAnimationInfo:nil
                            keyframesProvider:nil
                               reuseKeyframes:NO
                                  animationID:command.animation_id
                                   generation:command.generation
                                       cancel:YES];
      continue;
    }

    LynxAnimationInfo* info = MakeAnimationInfo(command);
    if (info == nil || command.properties.empty()) {
      continue;
    }
    auto properties = command.properties;
    auto animation_data = command.animation_data;
    LynxTypedKeyframesProvider provider = ^LynxKeyframeParsedData*(LynxUI* target) {
      LynxKeyframeParsedData* parsed_data = [[LynxKeyframeParsedData alloc] init];
      for (const auto& property : properties) {
        LynxKeyframeParsedData* property_data =
            MakeTypedKeyframes(property, animation_data, target);
        if (property_data == nil) {
          return nil;
        }
        MergeParsedKeyframes(parsed_data, property_data);
      }
      return parsed_data;
    };
    [ui prepareKeyframeManager];
    [ui.animationManager applyAnimationInfo:info
                          keyframesProvider:provider
                             reuseKeyframes:command.reuse_keyframes
                                animationID:command.animation_id
                                 generation:command.generation
                                     cancel:NO];
  }
}

}  // namespace lynx::tasm
