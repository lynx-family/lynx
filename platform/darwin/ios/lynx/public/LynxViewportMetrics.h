// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWPORTMETRICS_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWPORTMETRICS_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Lynx/LynxViewEnum.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxViewportMetrics : NSObject

@property(nonatomic, assign, readonly) CGSize size;
@property(nonatomic, assign, readonly) LynxViewSizeMode widthMode;
@property(nonatomic, assign, readonly) LynxViewSizeMode heightMode;

- (instancetype)initWithSize:(CGSize)size
                   widthMode:(LynxViewSizeMode)widthMode
                  heightMode:(LynxViewSizeMode)heightMode;

@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWPORTMETRICS_H_
