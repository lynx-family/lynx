// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBoxShadowManager.h"
#import "LynxColorUtils.h"
#import "LynxConverter+UI.h"
#import "LynxUI.h"
#import "LynxUnitUtils.h"

@implementation LynxBoxShadow

- (BOOL)isEqualToBoxShadow:(LynxBoxShadow*)other {
  if (self == other) return YES;
  if (![_shadowColor isEqual:other.shadowColor]) return NO;
  if (_offsetX != other.offsetX) return NO;
  if (_offsetY != other.offsetY) return NO;
  if (_blurRadius != other.blurRadius) return NO;
  if (_spreadRadius != other.spreadRadius) return NO;
  if (_inset != other.inset) return NO;
  return YES;
}

@end

@implementation LynxConverter (LynxBoxShadow)

+ (NSArray<LynxBoxShadow*>*)toLynxBoxShadow:(id)value {
  if (!value || ![value isKindOfClass:[NSArray class]]) {
    return nil;
  }
  NSMutableArray<LynxBoxShadow*>* result = [[NSMutableArray alloc] init];
  for (NSArray* item in value) {
    LynxBoxShadow* shadow = [[LynxBoxShadow alloc] init];
    shadow.offsetX = [item[0] doubleValue];
    shadow.offsetY = [item[1] doubleValue];
    shadow.blurRadius = [item[2] doubleValue];
    shadow.spreadRadius = [item[3] doubleValue];
    LynxBoxShadowOption option = (LynxBoxShadowOption)[item[4] intValue];
    if (option == LynxBoxShadowOptionInset) {
      shadow.inset = true;
    }
    shadow.inset = [item[4] doubleValue];
    shadow.shadowColor = [LynxConverter toUIColor:item[5]];
    [result addObject:shadow];
  }
  return result;
}

@end
