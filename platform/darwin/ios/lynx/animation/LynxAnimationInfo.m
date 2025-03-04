// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxAnimationInfo.h"
#import "LynxConverter+LynxCSSType.h"
#import "LynxLog.h"

@implementation LynxAnimationInfo
- (instancetype)initWithName:(NSString *)name {
  if (self = [super init]) {
    _prop = NONE;
    _duration = 0.0;
    _delay = 0.0;
    _timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
    _name = name;
  }
  return self;
}

- (BOOL)isEqual:(id)object {
  if (self == object) {
    return YES;
  }

  if (![object isKindOfClass:[LynxAnimationInfo class]]) {
    return NO;
  }
  return self.prop == [object prop];
}

// LynxAnimationInfo can be used as css animation(keyframe), layout animation, transition's info.
// This function is only used for comparison of css animation(keyframe) info.
- (BOOL)isEqualToKeyframeInfo:(LynxAnimationInfo *)info {
  if (self == info) {
    return YES;
  }
  if (info == nil) {
    return NO;
  }
  return [info.name isEqualToString:_name] && info.duration == _duration && info.delay == _delay &&
         [info.timingFunction isEqual:_timingFunction] && info.iterationCount == _iterationCount &&
         info.direction == _direction && [info.fillMode isEqualToString:_fillMode] &&
         info.playState == _playState;
}

- (BOOL)isOnlyPlayStateChanged:(LynxAnimationInfo *)info {
  if (self == info) {
    return NO;
  }
  if (info == nil) {
    return NO;
  }
  return [info.name isEqualToString:_name] && info.duration == _duration && info.delay == _delay &&
         [info.timingFunction isEqual:_timingFunction] && info.iterationCount == _iterationCount &&
         info.direction == _direction && [info.fillMode isEqualToString:_fillMode] &&
         info.playState != _playState;
}

- (NSUInteger)hash {
  NSString *toHashString = [NSString stringWithFormat:@"%lu", (unsigned long)_prop];
  return [toHashString hash];
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@: %p; duration: %f; delay: %f; animationType: %li;>",
                                    NSStringFromClass([self class]), self, _duration, _delay,
                                    (long)_timingFunction];
}

- (id)copyWithZone:(NSZone *)zone {
  LynxAnimationInfo *lai = [[LynxAnimationInfo allocWithZone:zone] init];
  lai.duration = self.duration;
  lai.delay = self.delay;
  lai.prop = self.prop;
  lai.timingFunction = self.timingFunction;
  lai.name = [self.name mutableCopy];
  lai.iterationCount = self.iterationCount;
  lai.direction = self.direction;
  lai.fillMode = self.fillMode;
  lai.playState = self.playState;
  lai.completeBlock = self.completeBlock;
  lai.orderIndex = self.orderIndex;
  return lai;
}

+ (BOOL)isDirectionReverse:(LynxAnimationInfo *)info {
  return info.direction == LynxAnimationDirectionReverse ||
         info.direction == LynxAnimationDirectionAlternateReverse;
}

+ (BOOL)isDirectionAlternate:(LynxAnimationInfo *)info {
  return info.direction == LynxAnimationDirectionAlternate ||
         info.direction == LynxAnimationDirectionAlternateReverse;
}

+ (BOOL)isFillModeRemoved:(LynxAnimationInfo *)info {
  return info.fillMode == kCAFillModeRemoved || info.fillMode == kCAFillModeBackwards;
}

+ (LynxAnimationInfo *)copyAnimationInfo:(LynxAnimationInfo *)info
                                withProp:(LynxAnimationProp)prop {
  LynxAnimationInfo *newInfo = [info copy];
  newInfo.prop = prop;
  newInfo.name = [LynxConverter toLynxPropName:newInfo.prop];
  return newInfo;
}

// TODO(WUJINTIAN): Add unit test for this method.
// When both lhsKey and rhsKey exist, keep the one that was added to animationInfos later.
+ (void)removeDuplicateAnimation:
            (NSMutableDictionary<NSNumber *, LynxAnimationInfo *> *)animationInfos
                         withKey:(LynxAnimationProp)lhsKey
                       sameToKey:(LynxAnimationProp)rhsKey {
  LynxAnimationInfo *lhsInfo = [animationInfos objectForKey:[NSNumber numberWithInt:(int)lhsKey]];
  LynxAnimationInfo *rhsInfo = [animationInfos objectForKey:[NSNumber numberWithInt:(int)rhsKey]];
  if (lhsInfo != nil && rhsInfo != nil) {
    if (lhsInfo.orderIndex < rhsInfo.orderIndex) {
      [animationInfos removeObjectForKey:[NSNumber numberWithInt:(int)lhsKey]];
    } else {
      [animationInfos removeObjectForKey:[NSNumber numberWithInt:(int)rhsKey]];
    }
  }
}

// TODO(WUJINTIAN): Add unit test for this method.
// Make position and size animation timing info consistent.
// We choose the one with a later end time
+ (void)makePositionAndSizeTimingInfoConsistent:
            (NSMutableDictionary<NSNumber *, LynxAnimationInfo *> *)animationInfos
                                withPositionKey:(LynxAnimationProp)positionKey
                                    withSizeKey:(LynxAnimationProp)sizeKey {
  LynxAnimationProp finalPositionKey = NONE;

  if (positionKey == TRANSITION_LAYOUT_POSITION_X) {
    bool isLeftExist =
        ([animationInfos objectForKey:[NSNumber numberWithInt:TRANSITION_LEFT]] != nil);
    finalPositionKey = isLeftExist ? TRANSITION_LEFT : TRANSITION_RIGHT;
  }
  if (positionKey == TRANSITION_LAYOUT_POSITION_Y) {
    bool isTopExist =
        ([animationInfos objectForKey:[NSNumber numberWithInt:TRANSITION_TOP]] != nil);
    finalPositionKey = isTopExist ? TRANSITION_TOP : TRANSITION_BOTTOM;
  }
  LynxAnimationInfo *positionInfo =
      [animationInfos objectForKey:[NSNumber numberWithInt:(int)finalPositionKey]];
  LynxAnimationInfo *sizeInfo = [animationInfos objectForKey:[NSNumber numberWithInt:(int)sizeKey]];

  LynxAnimationInfo *unifiedInfo =
      positionInfo.duration + positionInfo.delay < sizeInfo.duration + sizeInfo.delay
          ? sizeInfo
          : positionInfo;

  animationInfos[[NSNumber numberWithInt:(int)finalPositionKey]] =
      [LynxAnimationInfo copyAnimationInfo:unifiedInfo withProp:finalPositionKey];
  animationInfos[[NSNumber numberWithInt:(int)sizeKey]] =
      [LynxAnimationInfo copyAnimationInfo:unifiedInfo withProp:sizeKey];
}

@end

@implementation LynxConverter (LynxAnimationInfo)
+ (LynxAnimationInfo *)toKeyframeAnimationInfo:(id)value {
  if (!value || [value isEqual:[NSNull null]] || ![value isKindOfClass:[NSArray class]]) {
    return nil;
  }
  NSArray *array = (NSArray *)value;
  if ([value count] != 13) {
    LLogError(@"lynx animationInfo format error!");
    return nil;
  }
  LynxAnimationInfo *info = [[LynxAnimationInfo alloc] init];
  int index = 0;
  info.name = [LynxConverter toNSString:array[index++]];
  info.duration = [LynxConverter toNSTimeInterval:array[index++]];
  info.timingFunction =
      [LynxConverter toCAMediaTimingFunction:[array subarrayWithRange:NSMakeRange(index, 6)]];
  index += 6;
  info.delay = [LynxConverter toNSTimeInterval:array[index++]];
  info.iterationCount = [LynxConverter toint:array[index++]];
  info.direction = [LynxConverter toLynxAnimationDirectionType:array[index++]];
  info.fillMode = [LynxConverter toCAMediaTimingFillMode:array[index++]];
  info.playState = [LynxConverter toLynxAnimationPlayStateType:array[index]];
  return info;
}

+ (LynxAnimationInfo *)toTransitionAnimationInfo:(id)value {
  if (!value || [value isEqual:[NSNull null]] || ![value isKindOfClass:[NSArray class]]) {
    return nil;
  }
  NSArray *item = (NSArray *)value;
  if ([item count] != 9) {
    LLogError(@"lynx transition animationInfo format error!");
    return nil;
  }
  LynxAnimationInfo *info = [[LynxAnimationInfo alloc] init];
  info.prop = [LynxConverter toLynxAnimationProp:item[0]];
  if (info.prop == NONE) {
    return info;
  }
  info.name = [LynxConverter toLynxPropName:info.prop];
  info.duration = [LynxConverter toNSTimeInterval:item[1]];
  info.timingFunction =
      [LynxConverter toCAMediaTimingFunction:[item subarrayWithRange:NSMakeRange(2, 6)]];
  info.delay = [LynxConverter toNSTimeInterval:item[8]];
  return info;
}
@end

@implementation LynxConverter (LynxAnimationProp)
+ (LynxAnimationProp)toLynxAnimationProp:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return OPACITY;
  }
  return (LynxAnimationProp)[value intValue];
}

+ (NSString *)toLynxPropName:(LynxAnimationProp)prop {
  if (prop == OPACITY) {
    return @"transition-opacity";
  } else if (prop == SCALE_X) {
    return @"transition-scale-x";
  } else if (prop == SCALE_Y) {
    return @"transition-scale-y";
  } else if (prop == SCALE_XY) {
    return @"transition-scale-xy";
  } else if (prop == TRANSITION_WIDTH) {
    return @"transition-width";
  } else if (prop == TRANSITION_HEIGHT) {
    return @"transition-height";
  } else if (prop == TRANSITION_BACKGROUND_COLOR) {
    return @"transition-background-color";
  } else if (prop == TRANSITION_VISIBILITY) {
    return @"transition-visibility";
  } else if (prop == TRANSITION_LEFT) {
    return @"transition-left";
  } else if (prop == TRANSITION_RIGHT) {
    return @"transition-right";
  } else if (prop == TRANSITION_TOP) {
    return @"transition-top";
  } else if (prop == TRANSITION_BOTTOM) {
    return @"transition-bottom";
  } else if (prop == TRANSITION_TRANSFORM) {
    return @"transition-transform";
  } else if (prop == TRANSITION_ALL || prop == TRANSITION_LEGACY_ALL_1 ||
             prop == TRANSITION_LEGACY_ALL_2 || prop == TRANSITION_LEGACY_ALL_3) {
    return @"transition-all";
  } else {
    return @"transition-none";
  }
}

@end
