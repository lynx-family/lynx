// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXCONVERTER_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXCONVERTER_H_

#import <Foundation/Foundation.h>
#import "LynxCSSType.h"
/**
 * A class helping to convert value to the wanted type, you can add
 * custom conversion by category.
 *
 * When value is nil or equal to NSNull, you should return the default value.
 *
 * Example for UIColor conversion:
 *  + (UIColor*)toUIColor:(id)value
 */
@interface LynxConverter : NSObject

+ (CGFloat)toCGFloat:(id)value;
+ (NSInteger)toNSInteger:(id)value;
+ (int)toint:(id)value;
+ (NSUInteger)toNSUInteger:(id)value;
+ (NSString *)toNSString:(id)value;
+ (CGColorRef)toCGColorRef:(id)value;
+ (BOOL)toBOOL:(id)value;
+ (NSTimeInterval)toNSTimeInterval:(id)value;
+ (NSNumber *)toNSNumber:(id)value;
+ (id)toid:(id)value;
@end

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXCONVERTER_H_
