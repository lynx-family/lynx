// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_animation_darwin.h"

#import <Lynx/LynxAnimationInfo.h>
#import <Lynx/LynxKeyframeAnimator.h>
#import <Lynx/LynxPlatformAnimation.h>
#import <Lynx/LynxTransformRaw.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>

#include "gfx/animation/animation_timing.h"

namespace lynx::tasm {
namespace {
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
  // The evaluator normalizes supported iOS timing to linear or cubic-bezier.
  const auto& timing = data.timing_func;
  if (timing.timing_func != gfx::TimingFunctionType::kLinear &&
      timing.timing_func != gfx::TimingFunctionType::kCubicBezier) {
    return nil;
  }
  LynxAnimationInfo* info = [[LynxAnimationInfo alloc] initWithName:name];
  // initWithName: already supplies the linear timing function.
  if (timing.timing_func == gfx::TimingFunctionType::kCubicBezier) {
    info.timingFunction =
        [CAMediaTimingFunction functionWithControlPoints:timing.x1:timing.y1:timing.x2:timing.y2];
  }
  info.duration = static_cast<NSTimeInterval>(data.duration) / 1000.0;
  info.delay = static_cast<NSTimeInterval>(data.delay) / 1000.0;
  info.iterationCount = data.iteration_count;
  info.direction = static_cast<LynxAnimationDirectionType>(data.direction);
  info.fillMode = ToCAMediaTimingFillMode(data.fill_mode);
  info.playState = static_cast<LynxAnimationPlayStateType>(data.play_state);
  return info;
}

LynxPlatformLength* ToLynxLength(const gfx::LengthValue& value) {
  // gfx stores CSS percentages as percentage points. LynxPlatformLength uses
  // fractions for percentage transforms.
  const bool percent = value.unit == gfx::LengthUnit::kPercent;
  return [[LynxPlatformLength alloc]
      initWithValue:@(percent ? value.value / 100.0f : value.value)
               type:percent ? LynxPlatformLengthUnitPercentage : LynxPlatformLengthUnitNumber];
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
      raw = [[LynxTransformRaw alloc] initWithTranslationX:ToLynxLength(operation.translate.x)
                                                         y:ToLynxLength(operation.translate.y)
                                                         z:ToLynxLength(operation.translate.z)];
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

LynxKeyframeParsedData* MakeTypedKeyframes(const gfx::PlatformAnimationProperty& property,
                                           const gfx::AnimationData& animation_data, LynxUI* ui) {
  const bool transform = property.property == gfx::AnimationPropertyType::kTransform;
  if ((!transform && property.property != gfx::AnimationPropertyType::kOpacity) ||
      property.keyframes.size() < 2 || (transform && ui == nil)) {
    return nil;
  }
  const bool reverse = animation_data.direction == gfx::AnimationDirectionType::kReverse ||
                       animation_data.direction == gfx::AnimationDirectionType::kAlternateReverse;
  NSMutableArray* values = [[NSMutableArray alloc] initWithCapacity:property.keyframes.size()];
  NSMutableArray<NSNumber*>* times =
      [[NSMutableArray alloc] initWithCapacity:property.keyframes.size()];
  LynxKeyframeParsedData* parsed = transform ? nil : [[LynxKeyframeParsedData alloc] init];
  const auto value_type =
      transform ? gfx::KeyframeValueType::kTransform : gfx::KeyframeValueType::kFloat;
  const auto& keyframes = property.keyframes;
  for (size_t i = 0; i < keyframes.size(); ++i) {
    const auto& keyframe = keyframes[reverse ? keyframes.size() - 1 - i : i];
    if (!keyframe || keyframe->IsEmpty() || keyframe->ValueType() != value_type) {
      return nil;
    }
    id value;
    if (transform) {
      const auto* frame = static_cast<const gfx::TransformKeyframe*>(keyframe.get());
      if (!frame->HasResolvedValue()) {
        return nil;
      }
      value = ToLynxTransformRaw(frame->ResolvedValue());
    } else {
      value = @(static_cast<const gfx::FloatKeyframe*>(keyframe.get())->Value());
      if (keyframe->Offset() == 0.0) {
        parsed.beginStyles[@"opacity"] = value;
      } else if (keyframe->Offset() == 1.0) {
        parsed.endStyles[@"opacity"] = value;
      }
    }
    [values addObject:value];
    [times addObject:@(reverse ? 1.0 - keyframe->Offset() : keyframe->Offset())];
  }
  if (transform) {
    parsed = [LynxKeyframeAnimator buildParsedTransformKeyframes:values times:times ui:ui];
    if (reverse) {
      // Playback values are reversed, but fill styles use the original CSS endpoints.
      NSMutableDictionary* beginStyles = parsed.beginStyles;
      parsed.beginStyles = parsed.endStyles;
      parsed.endStyles = beginStyles;
    }
  } else {
    parsed.keyframeValues[@"opacity"] = values;
    parsed.keyframeTimes[@"opacity"] = times;
  }
  return parsed;
}

void MergeParsedKeyframes(LynxKeyframeParsedData* target, LynxKeyframeParsedData* source) {
  [target.keyframeValues addEntriesFromDictionary:source.keyframeValues];
  [target.keyframeTimes addEntriesFromDictionary:source.keyframeTimes];
  [target.beginStyles addEntriesFromDictionary:source.beginStyles];
  [target.endStyles addEntriesFromDictionary:source.endStyles];
  target.isPercentTransform |= source.isPercentTransform;
}

LynxPlatformAnimationPlayback* MakePlatformPlayback(
    const std::shared_ptr<const gfx::PlatformAnimationCommand> command) {
  if (!command->playback) {
    return nil;
  }
  LynxPlatformAnimationPlayback* platformPlayback = [LynxPlatformAnimationPlayback new];
  platformPlayback.currentTime = ^NSTimeInterval {
    return command->playback->CurrentTime().ToSecondsF();
  };
  platformPlayback.isCurrentExecutor = ^BOOL {
    return command->playback->IsCurrentExecutor(command->executor);
  };
  platformPlayback.claimEvent = ^BOOL(NSString* event) {
    using EventState = gfx::AnimationPlaybackState::EventState;
    const auto& playback = command->playback;
    const auto executor = command->executor;
    if ([event isEqualToString:@"animationstart"]) {
      const auto& animation_data = command->animation_data;
      gfx::AnimationTimingInput timing;
      timing.animation_data = &animation_data;
      timing.duration = fml::TimeDelta::FromMilliseconds(animation_data.duration);
      // A finished animation can become active after a timing update, but
      // rebuilding its completed fill is not a new logical start.
      const bool active =
          gfx::CalculatePhase(timing, playback->CurrentTime()) == gfx::TimingPhase::ACTIVE;
      return playback->ClaimEvent(executor, EventState::kStarted, active);
    } else if ([event isEqualToString:@"animationend"]) {
      return playback->ClaimEvent(executor, EventState::kFinished);
    } else if ([event isEqualToString:@"animationcancel"]) {
      return playback->ClaimEvent(executor, EventState::kCanceled);
    } else if ([event isEqualToString:@"animationiteration"]) {
      // TODO: Integrate iteration event tracking when the iOS animator
      // supports it. Iterations need boundary-based deduplication across
      // executor switches and must not change the lifecycle event state.
      return NO;
    } else {
      // Unknown events must not change the logical animation's event state.
      return NO;
    }
  };
  return platformPlayback;
}

}  // namespace

void ApplyPlatformAnimationCommands(
    LynxUI* ui, const std::shared_ptr<gfx::PlatformAnimationCommandBatch>& commands) {
  if (ui == nil || commands == nullptr || commands->empty()) {
    return;
  }

  for (const auto& command : *commands) {
    if (command.kind != gfx::AnimationKind::kKeyframe) {
      continue;
    }
    const bool cancel = command.type == gfx::PlatformAnimationCommandType::kCancel;
    const bool detach = command.type == gfx::PlatformAnimationCommandType::kDetach;
    if (cancel || detach) {
      [ui.animationManager applyAnimationInfo:nil
                            keyframesProvider:nil
                               reuseKeyframes:NO
                                  animationID:command.animation_id
                                   generation:command.generation
                                       cancel:cancel
                                       detach:detach
                                     playback:nil];
      continue;
    }

    if (command.playback && !command.playback->IsCurrentExecutor(command.executor)) {
      continue;
    }
    LynxAnimationInfo* info = MakeAnimationInfo(command);
    if (info == nil || command.properties.empty()) {
      continue;
    }
    // Keep the immutable command alive with its batch; providers can run again
    // after this call when a percentage transform needs the new view size.
    const std::shared_ptr<const gfx::PlatformAnimationCommand> retained_command(commands, &command);

    LynxTypedKeyframesProvider provider = ^LynxKeyframeParsedData*(LynxUI* target) {
      LynxKeyframeParsedData* parsed_data = nil;
      for (const auto& property : retained_command->properties) {
        LynxKeyframeParsedData* property_data =
            MakeTypedKeyframes(property, retained_command->animation_data, target);
        if (property_data == nil) {
          return nil;
        }
        if (parsed_data == nil) {
          parsed_data = property_data;
        } else {
          MergeParsedKeyframes(parsed_data, property_data);
        }
      }
      return parsed_data;
    };
    [ui prepareKeyframeManager];
    [ui.animationManager applyAnimationInfo:info
                          keyframesProvider:provider
                             reuseKeyframes:command.reuse_keyframes
                                animationID:command.animation_id
                                 generation:command.generation
                                     cancel:NO
                                     detach:NO
                                   playback:MakePlatformPlayback(retained_command)];
  }
}

}  // namespace lynx::tasm
