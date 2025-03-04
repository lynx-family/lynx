// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFilterUtil.h"

@implementation LynxFilterUtil (System)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-protocol-method-implementation"

+ (id)getFilterWithType:(LynxFilterType)type filterAmount:(CGFloat)filter_amount {
  // private api
  NSString* clsName = @"CAFilter";
  Class clz = NSClassFromString(clsName);
  if ([clz respondsToSelector:@selector(filterWithName:)]) {
    NSString* filterName = nil;
    NSString* keyPath = nil;
    switch (type) {
      case LynxFilterTypeGrayScale:
        filterName = @"colorSaturate";
        keyPath = @"inputAmount";
        break;
      case LynxFilterTypeBlur:
        filterName = @"gaussianBlur";
        keyPath = @"inputRadius";
        break;
      default:
        // No such filter
        return nil;
    };
    id filter = [clz filterWithName:filterName];
    [filter setValue:[NSNumber numberWithFloat:filter_amount] forKey:keyPath];
    return filter;
  }
  // Api get failed.
  return nil;
}

#pragma clang diagnostic pop

@end
