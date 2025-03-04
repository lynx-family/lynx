// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_UTILS_LYNXUNITUTILS_H_
#define DARWIN_COMMON_LYNX_UTILS_LYNXUNITUTILS_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#import "LynxScreenMetrics.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUnitUtils : NSObject
+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue;
+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue withDefaultPt:(CGFloat)defaultPt;
+ (CGFloat)toPtFromIDUnitValue:(id)unitValue withDefaultPt:(CGFloat)defaultPt;
+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue
                rootFontSize:(CGFloat)rootFontSize
                 curFontSize:(CGFloat)curFontSize
                   rootWidth:(int)rootWidth
                  rootHeight:(int)rootHeight;
+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue
                rootFontSize:(CGFloat)rootFontSize
                 curFontSize:(CGFloat)curFontSize
                   rootWidth:(int)rootWidth
                  rootHeight:(int)rootHeight
               withDefaultPt:(CGFloat)defaultPt;
+ (CGFloat)toPtWithScreenMetrics:(LynxScreenMetrics *)screenMetrics
                       unitValue:(NSString *)unitValue
                    rootFontSize:(CGFloat)rootFontSize
                     curFontSize:(CGFloat)curFontSize
                       rootWidth:(int)rootWidth
                      rootHeight:(int)rootHeight
                        viewSize:(CGFloat)viewSize
                   withDefaultPt:(CGFloat)defaultPt;
+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue
                rootFontSize:(CGFloat)rootFontSize
                 curFontSize:(CGFloat)curFontSize
                   rootWidth:(int)rootWidth
                  rootHeight:(int)rootHeight
                    viewSize:(CGFloat)viewSize
               withDefaultPt:(CGFloat)defaultPt;
+ (CGFloat)toPtWithScreenMetrics:(LynxScreenMetrics *)screenMetrics
                       unitValue:(NSString *)unitValue
                    rootFontSize:(CGFloat)rootFontSize
                     curFontSize:(CGFloat)curFontSize
                       rootWidth:(int)rootWidth
                      rootHeight:(int)rootHeight
                   withDefaultPt:(CGFloat)defaultPt;
+ (CGFloat)toPhysicalPixelFromPt:(CGFloat)valuePt;
+ (BOOL)isPercentage:(NSString *)unitValue;

+ (CGFloat)clamp:(CGFloat)value min:(CGFloat)minValue max:(CGFloat)maxValue;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_UTILS_LYNXUNITUTILS_H_
