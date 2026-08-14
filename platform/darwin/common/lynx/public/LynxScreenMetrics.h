// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXSCREENMETRICS_H_
#define DARWIN_COMMON_LYNX_LYNXSCREENMETRICS_H_

#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxScreenMetrics : NSObject
@property(nonatomic, assign, readonly) CGSize screenSize;
@property(nonatomic, assign, readonly) CGFloat scale;

+ (LynxScreenMetrics*)getDefaultLynxScreenMetrics;
- (instancetype)initWithScreenSize:(CGSize)screenSize scale:(CGFloat)scale;
- (void)setScreenSize:(CGSize)screenSize
    __attribute__((deprecated("Use initWithScreenSize:scale: to create an immutable snapshot.")));
- (void)setScale:(CGFloat)scale
    __attribute__((deprecated("Use initWithScreenSize:scale: to create an immutable snapshot.")));
- (void)setLynxScreenSize:(CGSize)screenSize
    __attribute__((deprecated("Use initWithScreenSize:scale: to create an immutable snapshot.")));

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_LYNXSCREENMETRICS_H_
