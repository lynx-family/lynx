// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxConverter+UI.h"

@implementation LynxConverter (UI)

+ (COLOR_CLASS *)toUIColor:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return nil;
  }
  if ([value isKindOfClass:[NSNumber class]]) {
    NSUInteger argb = [self toNSUInteger:value];
    CGFloat a = ((argb >> 24) & 0xFF) / 255.0;
    CGFloat r = ((argb >> 16) & 0xFF) / 255.0;
    CGFloat g = ((argb >> 8) & 0xFF) / 255.0;
    CGFloat b = (argb & 0xFF) / 255.0;
    return [COLOR_CLASS colorWithRed:r green:g blue:b alpha:a];
  } else {
    NSAssert(false, @"%@ is not a UIColor", value);
    return nil;
  }
}

// TODO (xiamengfei.moonface): move to iOS folder
#if TARGET_OS_IOS
+ (UIAccessibilityTraits)toAccessibilityTraits:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return UIAccessibilityTraitNone;
  }
  NSString *valueStr = [LynxConverter toNSString:value];
  NSArray<NSString *> *traitStrings = [valueStr componentsSeparatedByString:@","];
  UIAccessibilityTraits traits = UIAccessibilityTraitNone;
  for (NSString *traitString in traitStrings) {
    if ([traitString isEqualToString:@"text"]) {
      traits |= UIAccessibilityTraitStaticText;
    } else if ([traitString isEqualToString:@"image"]) {
      traits |= UIAccessibilityTraitImage;
    } else if ([traitString isEqualToString:@"button"]) {
      traits |= UIAccessibilityTraitButton;
    } else if ([traitString isEqualToString:@"link"]) {
      traits |= UIAccessibilityTraitLink;
    } else if ([traitString isEqualToString:@"header"]) {
      traits |= UIAccessibilityTraitHeader;
    } else if ([traitString isEqualToString:@"search"]) {
      traits |= UIAccessibilityTraitSearchField;
    } else if ([traitString isEqualToString:@"selected"]) {
      traits |= UIAccessibilityTraitSelected;
    } else if ([traitString isEqualToString:@"playable"]) {
      traits |= UIAccessibilityTraitPlaysSound;
    } else if ([traitString isEqualToString:@"keyboard"]) {
      traits |= UIAccessibilityTraitKeyboardKey;
    } else if ([traitString isEqualToString:@"summary"]) {
      traits |= UIAccessibilityTraitSummaryElement;
    } else if ([traitString isEqualToString:@"disabled"]) {
      traits |= UIAccessibilityTraitNotEnabled;
    } else if ([traitString isEqualToString:@"updating"]) {
      traits |= UIAccessibilityTraitUpdatesFrequently;
    } else if ([traitString isEqualToString:@"adjustable"]) {
      traits |= UIAccessibilityTraitAdjustable;
    } else if ([traitString isEqualToString:@"tabbar"]) {
      if (@available(iOS 10.0, *)) {
        traits |= UIAccessibilityTraitTabBar;
      }
    }
  }
  return traits;
}
#endif

@end
