// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxColorUtils.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxPropsProcessor.h>
#import <XElement/LynxUIBlurEffect.h>
#import <XElement/LynxUIBlurView.h>

#if defined(__IPHONE_OS_VERSION_MAX_ALLOWED) && defined(__IPHONE_26_0) && \
    (__IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_26_0)
#import <UIKit/UIGlassEffect.h>
#define LYNX_GLASS_EFFECT_AVAILABLE 1
#else
#define LYNX_GLASS_EFFECT_AVAILABLE 0
#endif

@interface LynxUIBlurView ()
@property(nonatomic, assign) UIBlurEffectStyle style;
@property(nonatomic, assign) ushort mode;
@property(nonatomic, assign) CGFloat radius;
@property(nonatomic, assign) CGFloat spacing;
#if LYNX_GLASS_EFFECT_AVAILABLE
@property(nonatomic, assign) BOOL glassInteractive;
@property(nonatomic, strong, nullable) UIColor* glassTintColor;
@property(nonatomic, assign) UIGlassEffectStyle glassStyle;
#endif
@end

@implementation LynxUIBlurView {
  bool _shouldUpdateEffect;
}

NS_ENUM(ushort, LynxEffectMode) {
  LynxEffectModeBlur = 1,
#if LYNX_GLASS_EFFECT_AVAILABLE
  LynxEffectModeGlass = 2, LynxEffectModeGlassContainer = 3
#endif
};

- (instancetype)init {
  self = [super init];
  if (self) {
    _mode = LynxEffectModeBlur;
#if LYNX_GLASS_EFFECT_AVAILABLE
    _glassInteractive = NO;
    _glassTintColor = nil;
    _glassStyle = UIGlassEffectStyleRegular;
#endif
  }
  return self;
}

- (UIView*)createView {
  self.style = UIBlurEffectStyleLight;
  UIVisualEffectView* view = [[UIVisualEffectView alloc] init];
  return view;
}

- (void)insertChild:(LynxUI*)child atIndex:(NSInteger)index {
  [self didInsertChild:child atIndex:index];
  [self.view.contentView insertSubview:[child view] atIndex:index];
  if (index > 0) {
    LynxUI* ui = [self.children objectAtIndex:index - 1];
    CALayer* layer = ui.view.layer;
    if (index <= self.view.contentView.layer.sublayers.count &&
        layer != [self.view.contentView.layer.sublayers objectAtIndex:index - 1]) {
      CALayer* childLayer = [child view].layer;
      [childLayer removeFromSuperlayer];
      [self.view.contentView.layer insertSublayer:childLayer above:layer];
      LynxBackgroundManager* mgr = [child backgroundManager];
      if (mgr != nil) {
        if (mgr.borderLayer != nil) {
          [mgr.borderLayer removeFromSuperlayer];
          [self.view.contentView.layer insertSublayer:mgr.borderLayer above:childLayer];
        }
        if (mgr.backgroundLayer != nil) {
          [mgr.backgroundLayer removeFromSuperlayer];
          [self.view.contentView.layer insertSublayer:mgr.backgroundLayer below:childLayer];
        }
      }
    }
  }
}

LYNX_PROP_SETTER("blur-effect", setBlurEffect, NSString*) {
  if (requestReset) {
    value = @"light";
  }

  UIBlurEffectStyle style = UIBlurEffectStyleLight;
  enum LynxEffectMode mode = LynxEffectModeBlur;

  [self parseBlurEffectValue:value style:&style mode:&mode];

  if (self.style != style || self.mode != mode) {
    self.style = style;
    self.mode = mode;
    _shouldUpdateEffect = YES;
  }
}

LYNX_PROP_SETTER("background-color", setBackgroundColor, UIColor*) {}

LYNX_PROP_SETTER("background-origin", setBackgroundOrigin, NSString*) {}

LYNX_PROP_SETTER("background-position", setBackgroundPosition, NSString*) {}

LYNX_PROP_SETTER("background-repeat", setBackgroundRepeat, NSString*) {}

LYNX_PROP_SETTER("background-size", setBackgroundSize, NSString*) {}

LYNX_PROP_SETTER("background-capInsets", setBackgroundCapInsets, NSString*) {}

LYNX_PROP_SETTER("background-clip", setBackgroundClip, NSArray*) {}

LYNX_PROP_SETTER("background", setBackground, NSString*) {}

LYNX_PROP_SETTER("background-image", setBackgroundImage, NSString*) {}

/**
 * @name: blur-radius
 * @description: radius for gaussian filter
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.10
 **/
LYNX_PROP_SETTER("blur-radius", setBlurRadius, CGFloat) {
  if (requestReset) {
    value = 0.0;
  }
  if (self.radius != value) {
    self.radius = value;
    _shouldUpdateEffect = YES;
  }
}

/**
 * @name: spacing
 * @description: The spacing specifies the distance between elements at which
 *they begin to merge.
 * @supportVersion: 3.6
 **/
LYNX_PROP_SETTER("spacing", setSpacing, CGFloat) {
  if (requestReset) {
    value = 0.0;
  }
  if (self.spacing != value) {
    self.spacing = value;
    _shouldUpdateEffect = YES;
  }
}

#if LYNX_GLASS_EFFECT_AVAILABLE
/**
 * @name: glass-interactive
 * @description: Enables interactive behavior for the glass effect.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 3.8
 **/
LYNX_PROP_SETTER("glass-interactive", setGlassInteractive, BOOL) {
  if (requestReset) {
    value = NO;
  }
  if (self.glassInteractive != value) {
    self.glassInteractive = value;
    _shouldUpdateEffect = YES;
  }
}

/**
 * @name: glass-tint-color
 * @description: A tint color applied to the glass effect.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 3.8
 **/
LYNX_PROP_SETTER("glass-tint-color", setGlassTintColor, NSString*) {
  if (requestReset) {
    value = nil;
  }
  UIColor* newColor = [LynxColorUtils convertNSStringToUIColor:value];
  if (![self.glassTintColor isEqual:newColor]) {
    self.glassTintColor = newColor;
    _shouldUpdateEffect = YES;
  }
}

/**
 * @name: glass-style
 * @description: The style of the glass effect (regular or clear).
 * @category: different
 * @standardAction: keep
 * @supportVersion: 3.8
 **/
LYNX_PROP_SETTER("glass-style", setGlassStyle, NSString*) {
  if (requestReset) {
    value = @"regular";
  }
  UIGlassEffectStyle style = UIGlassEffectStyleRegular;
  if ([value isEqualToString:@"clear"]) {
    style = UIGlassEffectStyleClear;
  }
  if (self.glassStyle != style) {
    self.glassStyle = style;
    _shouldUpdateEffect = YES;
  }
}

LYNX_PROP_SETTER("ios-user-interface-style", setUserInterfaceStyle, NSString*) {
  UIUserInterfaceStyle style = UIUserInterfaceStyleUnspecified;

  if (!requestReset) {
    if ([value isEqualToString:@"dark"]) {
      style = UIUserInterfaceStyleDark;
    } else if ([value isEqualToString:@"light"]) {
      style = UIUserInterfaceStyleLight;
    }
  }

  if (@available(iOS 13.0, *)) {
    self.view.overrideUserInterfaceStyle = style;
  }
}

#endif

- (void)propsDidUpdate {
  if (!_shouldUpdateEffect) {
    return;
  }

  // Remove the current effect to ensure 'effectSettings' to be invoked. If
  // the effect style doesn't change, the 'effectSettings' will not be invoke
  // again.
  self.view.effect = nil;

  UIVisualEffect* effect = [self createEffectForCurrentMode];
  if (effect) {
    self.view.effect = effect;
  }
  _shouldUpdateEffect = NO;
}

- (void)parseBlurEffectValue:(NSString*)value
                       style:(UIBlurEffectStyle*)style
                        mode:(enum LynxEffectMode*)mode {
  // Use dictionary for O(1) lookup instead of chained if-else
  static NSDictionary<NSString*, NSNumber*>* blurEffectMap = nil;
#if LYNX_GLASS_EFFECT_AVAILABLE
  static NSDictionary<NSString*, NSNumber*>* glassEffectMap = nil;
#endif
  static dispatch_once_t onceToken;

  dispatch_once(&onceToken, ^{
    blurEffectMap = @{
      @"light" : @(UIBlurEffectStyleLight),
      @"dark" : @(UIBlurEffectStyleDark),
      @"extra-light" : @(UIBlurEffectStyleExtraLight)
    };

#if LYNX_GLASS_EFFECT_AVAILABLE
    if (@available(iOS 26, *)) {
      glassEffectMap = @{
        @"glass" : @(LynxEffectModeGlass),
        @"glass-container" : @(LynxEffectModeGlassContainer)
      };
    }
#endif
  });

  // Check for standard blur effects first
  NSNumber* styleNumber = blurEffectMap[value];
  if (styleNumber) {
    *style = [styleNumber integerValue];
    *mode = LynxEffectModeBlur;
    return;
  }

  // Check for glass effects (iOS 26+)
#if LYNX_GLASS_EFFECT_AVAILABLE
  if (@available(iOS 26, *)) {
    NSNumber* modeNumber = glassEffectMap[value];
    if (modeNumber) {
      *mode = [modeNumber integerValue];
      // Keep default style for glass effects
      return;
    }
  }
#endif

  // Default to light blur if unknown value
  LLogError(@"Lynx Prop setter error: wrong blur-effect '%@', use default "
            @"blur-effect light.",
            value);
  *style = UIBlurEffectStyleLight;
  *mode = LynxEffectModeBlur;
}

- (UIVisualEffect*)createEffectForCurrentMode {
  switch (self.mode) {
    case LynxEffectModeBlur:
      return [self createBlurEffect];
#if LYNX_GLASS_EFFECT_AVAILABLE
    case LynxEffectModeGlass:
      if (@available(iOS 26, *)) {
        UIGlassEffect* effect = [UIGlassEffect effectWithStyle:self.glassStyle];
        effect.interactive = self.glassInteractive;
        if (self.glassTintColor) {
          effect.tintColor = self.glassTintColor;
        }
        return effect;
      }
      return nil;
    case LynxEffectModeGlassContainer:
      return [self createGlassContainerEffect];
#endif
    default:
      return nil;
  }
}

- (UIBlurEffect*)createBlurEffect {
  // if radius is 0, remove the effect.
  if (self.radius == 0) {
    return nil;
  }
  return [LynxUIBlurEffect effectWithStyle:self.style blurRadius:self.radius];
}

#if LYNX_GLASS_EFFECT_AVAILABLE
- (UIGlassContainerEffect*)createGlassContainerEffect {
  if (@available(iOS 26, *)) {
    UIGlassContainerEffect* effect = [UIGlassContainerEffect new];
    effect.spacing = self.spacing;
    return effect;
  }
  return nil;
}
#endif

@end
