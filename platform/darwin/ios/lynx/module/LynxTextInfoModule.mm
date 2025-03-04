// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextInfoModule.h"
#import "LynxContext+Internal.h"
#import "LynxContext.h"
#import "LynxTextUtils.h"
#import "LynxThreadManager.h"
#import "LynxUIOwner.h"
#import "LynxUnitUtils.h"
#include "core/renderer/utils/ios/text_utils_ios.h"

using namespace lynx;

@implementation LynxTextInfoModule {
  __weak LynxContext *context_;
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
  }

  return self;
}

+ (NSString *)name {
  return @"LynxTextInfoModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{@"getTextInfo" : NSStringFromSelector(@selector(getTextInfo:params:))};
}

- (NSDictionary *)getTextInfo:(NSString *)text params:(NSDictionary *)info {
  if (text.length == 0) {
    return @{@"width" : @0.f};
  }
  NSString *font_size = [info objectForKey:@"fontSize"];
  static CGFloat kLynxDefaultFontSize = 14;
  CGFloat fontSize = kLynxDefaultFontSize;
  if (font_size && (font_size.length != 0)) {
    fontSize = [LynxUnitUtils toPtFromUnitValue:font_size withDefaultPt:kLynxDefaultFontSize];
  }
  NSString *fontFamily = [info objectForKey:@"fontFamily"];
  id max_line = [info objectForKey:@"maxLine"];
  NSInteger maxLine;
  if ([max_line isKindOfClass:[NSNumber class]]) {
    maxLine = [max_line integerValue];
  } else {
    maxLine = 1;
  }
  CGFloat maxWidth = 0;
  NSString *max_width = [info objectForKey:@"maxWidth"];
  if (max_width.length != 0) {
    maxWidth = [LynxUnitUtils toPtFromUnitValue:max_width withDefaultPt:kLynxDefaultFontSize];
  }
  NSDictionary *ret = [LynxTextUtils measureText:text
                                        fontSize:fontSize
                                      fontFamily:fontFamily
                                        maxWidth:maxWidth
                                         maxLine:maxLine];
  return ret;
}

@end
