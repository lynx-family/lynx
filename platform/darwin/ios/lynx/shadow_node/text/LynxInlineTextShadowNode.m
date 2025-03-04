// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxInlineTextShadowNode.h"
#import "LynxComponentRegistry.h"
#import "LynxPropsProcessor.h"

@implementation LynxInlineTextShadowNode

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("inline-text")
#else
LYNX_REGISTER_SHADOW_NODE("inline-text")
#endif

- (BOOL)isVirtual {
  return YES;
}

- (BOOL)needsEventSet {
  return YES;
}

LYNX_PROP_SETTER("vertical-align", setVerticalAlign, NSArray*) {
  [self setVerticalAlignOnShadowNode:requestReset value:value];
}

LYNX_PROP_SETTER("background-image", setBackgroundImage, NSArray*) {
  if (!self.textStyle.backgroundDrawable || requestReset) {
    self.textStyle.backgroundDrawable = [NSMutableArray new];
  }
  [self setBackgroundOrMaskWithDrawable:self.textStyle.backgroundDrawable
                                  reset:requestReset
                                  value:value];
}

- (void)setBackgroundOrMaskWithDrawable:(NSMutableArray*)drawableArr
                                  reset:(BOOL)reset
                                  value:(NSArray*)value {
  if (reset) {
    value = [NSArray new];
  }
  [drawableArr removeAllObjects];
  for (NSUInteger i = 0; i < [value count]; i++) {
    NSUInteger type = [LynxConverter toNSUInteger:[value objectAtIndex:i]];
    if (type == LynxBackgroundImageLinearGradient) {
      i++;
      LynxBackgroundLinearGradientDrawable* drawable =
          [[LynxBackgroundLinearGradientDrawable alloc] initWithArray:value[i]];
      drawable.repeatX = 1;
      drawable.repeatY = 1;
      [drawableArr addObject:drawable];
    } else {
      NSAssert(false, @"the background-image content not support");
    }
  }
  [self.textStyle updateBackgroundDrawableSize];
  [self.textStyle updateBackgroundDrawablePosition];
  [self.textStyle updateBackgroundDrawableRepeat];
}

LYNX_PROP_SETTER("background-size", setBackgroundSize, NSArray*) {
  [self setBackgroundOrMaskWithSize:self.textStyle.backgroundImageSize
                              reset:requestReset
                              value:value];
  [self.textStyle updateBackgroundDrawableSize];
}

- (void)setBackgroundOrMaskWithSize:(NSMutableArray*)size reset:(BOOL)reset value:(NSArray*)value {
  [size removeAllObjects];
  if (reset || value.count % 2 != 0) {
    value = [NSArray new];
  }
  for (NSUInteger i = 0; i < [value count]; i += 2) {
    LynxPlatformLength* length =
        [[LynxPlatformLength alloc] initWithValue:[value objectAtIndex:i]
                                             type:[[value objectAtIndex:i + 1] unsignedIntValue]];
    [size addObject:[[LynxBackgroundSize alloc] initWithLength:length]];
  }
}

LYNX_PROP_SETTER("background-position", setBackgroundPosition, NSArray*) {
  [self setBackgroundOrMaskWithPosition:self.textStyle.backgroundPosition
                                  reset:requestReset
                                  value:value];
  [self.textStyle updateBackgroundDrawablePosition];
}

- (void)setBackgroundOrMaskWithPosition:(NSMutableArray*)position
                                  reset:(BOOL)reset
                                  value:(NSArray*)value {
  [position removeAllObjects];
  if (reset || value.count % 2 != 0) {
    value = [NSArray new];
  }
  for (NSUInteger i = 0; i < [value count]; i += 2) {
    NSUInteger type = [LynxConverter toNSUInteger:[value objectAtIndex:i + 1]];
    LynxBackgroundPosition* backgroundPosition = NULL;
    LynxPlatformLength* len = [[LynxPlatformLength alloc] initWithValue:[value objectAtIndex:i]
                                                                   type:type];
    backgroundPosition = [[LynxBackgroundPosition alloc] initWithValue:len];
    [position addObject:backgroundPosition];
  }
}

LYNX_PROP_SETTER("background-repeat", setBackgroundRepeat, NSArray*) {
  [self setBackgroundOrMaskWithRepeat:self.textStyle.backgroundRepeat
                                reset:requestReset
                                value:value];
  [self.textStyle updateBackgroundDrawableRepeat];
}

- (void)setBackgroundOrMaskWithRepeat:(NSMutableArray*)repeat
                                reset:(BOOL)reset
                                value:(NSArray*)value {
  [repeat removeAllObjects];
  if (reset) {
    value = [NSArray array];
  }
  for (NSNumber* number in value) {
    [repeat addObject:@([number integerValue])];
  }
}

#define SET_BORDER_UNIT_VAL(dst, src, offset)                        \
  {                                                                  \
    newLength = NULL;                                                \
    LBRGetBorderValueOrLength(value, offset, &newValue, &newLength); \
    dst = src;                                                       \
  }

//@see computed_css_style.cc#borderRadiusToLepus.
LYNX_PROP_SETTER("border-radius", setBorderRadius, NSArray*) {
  if (requestReset) {
    value = @[ @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0 ];
  }
  LynxBorderRadii borderRadius = self.textStyle.borderRadius;

  LynxBorderUnitValue newValue = {0, LynxBorderValueUnitDefault};
  LynxPlatformLength* newLength = NULL;

  SET_BORDER_UNIT_VAL(borderRadius.topLeftX, newValue, 0)
  SET_BORDER_UNIT_VAL(borderRadius.topLeftY, newValue, 2)
  SET_BORDER_UNIT_VAL(borderRadius.topRightX, newValue, 4)
  SET_BORDER_UNIT_VAL(borderRadius.topRightY, newValue, 6)
  SET_BORDER_UNIT_VAL(borderRadius.bottomRightX, newValue, 8)
  SET_BORDER_UNIT_VAL(borderRadius.bottomRightY, newValue, 10)
  SET_BORDER_UNIT_VAL(borderRadius.bottomLeftX, newValue, 12)
  SET_BORDER_UNIT_VAL(borderRadius.bottomLeftY, newValue, 14)
  self.textStyle.borderRadius = borderRadius;
  [self.textStyle updateBackgroundRadius];
}

#define LYNX_PROP_SET_BORDER_RADIUS(position)                       \
  {                                                                 \
    if (requestReset) {                                             \
      value = @[ @0, @0, @0, @0 ];                                  \
    }                                                               \
    LynxBorderRadii borderRadius = self.textStyle.borderRadius;     \
                                                                    \
    LynxBorderUnitValue newValue = {0, LynxBorderValueUnitDefault}; \
    LynxPlatformLength* newLength = NULL;                           \
                                                                    \
    SET_BORDER_UNIT_VAL(borderRadius.position##X, newValue, 0)      \
    SET_BORDER_UNIT_VAL(borderRadius.position##Y, newValue, 2)      \
                                                                    \
    self.textStyle.borderRadius = borderRadius;                     \
    [self.textStyle updateBackgroundRadius];                        \
  }

LYNX_PROP_SETTER("border-top-left-radius", setBorderTopLeftRadius,
                 NSArray*){LYNX_PROP_SET_BORDER_RADIUS(topLeft)}

LYNX_PROP_SETTER("border-bottom-left-radius", setBorderBottomLeftRadius,
                 NSArray*){LYNX_PROP_SET_BORDER_RADIUS(bottomLeft)}

LYNX_PROP_SETTER("border-top-right-radius", setBorderTopRightRadius,
                 NSArray*){LYNX_PROP_SET_BORDER_RADIUS(topRight)}

LYNX_PROP_SETTER("border-bottom-right-radius", setBorderBottomRightRadius, NSArray*) {
  LYNX_PROP_SET_BORDER_RADIUS(bottomRight)
}

@end
