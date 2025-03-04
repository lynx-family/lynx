// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_UTILS_LYNXSIZEVALUE_H_
#define DARWIN_COMMON_LYNX_UTILS_LYNXSIZEVALUE_H_

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LynxSizeValueType) {
  LynxSizeValueTypeUnknown = 0,   // unknown
  LynxSizeValueTypePercent = 1,   // %
  LynxSizeValueTypeDevicePt = 2,  // device pt, equal to CSS px on iOS
};

@interface LynxSizeValue : NSObject
@property(nonatomic, assign) LynxSizeValueType type;
@property(nonatomic, assign) CGFloat value;

+ (nullable instancetype)sizeValueFromCSSString:(nullable NSString *)valueStr;
- (instancetype)initWithType:(LynxSizeValueType)type value:(CGFloat)value;
- (CGFloat)convertToDevicePtWithFullSize:(CGFloat)fullSize;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_UTILS_LYNXSIZEVALUE_H_
