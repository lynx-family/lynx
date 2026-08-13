// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUIMarkdownShadowNodeV2;
@class MarkdownMeasurer;

@interface LynxMarkdownBundleV2 : NSObject

@property(nonatomic, strong, readonly, nullable) MarkdownMeasurer *markdownMeasurer;
@property(nonatomic, weak, readonly, nullable) LynxUIMarkdownShadowNodeV2 *shadowNode;
@property(nonatomic, assign, readonly) CGSize measuredSize;

- (instancetype)initWithMarkdownMeasurer:(nullable MarkdownMeasurer *)markdownMeasurer
                              shadowNode:(nullable LynxUIMarkdownShadowNodeV2 *)shadowNode
                            measuredSize:(CGSize)measuredSize;

@end

NS_ASSUME_NONNULL_END
