// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "UIHelper.h"

@implementation UIHelper

+ (UIViewController *)getTopViewController {
  //  getting rootViewController
  UIViewController *topController = [UIApplication sharedApplication].keyWindow.rootViewController;
  //  getting topMost ViewController
  while (topController != nil) {
    if ([topController isKindOfClass:[UINavigationController class]]) {
      return topController;
    }
    topController = [topController presentedViewController];
  }
  return nil;
}

+ (UIColor *)colorWithHexString:(NSString *)hexString {
  if ([hexString hasPrefix:@"#"]) {
    hexString = [hexString substringFromIndex:1];
  }

  if (hexString.length == 6 || hexString.length == 8) {
    unsigned int hexValue;
    [[NSScanner scannerWithString:hexString] scanHexInt:&hexValue];

    CGFloat red, green, blue, alpha;

    // handle RGB or RGBA
    if (hexString.length == 6) {
      red = ((hexValue >> 16) & 0xFF) / 255.0;
      green = ((hexValue >> 8) & 0xFF) / 255.0;
      blue = (hexValue & 0xFF) / 255.0;
      alpha = 1.0;
    } else {
      red = ((hexValue >> 24) & 0xFF) / 255.0;
      green = ((hexValue >> 16) & 0xFF) / 255.0;
      blue = ((hexValue >> 8) & 0xFF) / 255.0;
      alpha = (hexValue & 0xFF) / 255.0;
    }
    return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
  }

  return [UIColor clearColor];
}

@end
