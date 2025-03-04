// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import "LynxCSSType.h"

@interface LynxLayoutStyle : NSObject

@property(nonatomic, readonly) NSInteger sign;

- (instancetype)initWithSign:(NSInteger)sign layoutNodeManager:(void*)layoutNodeManager;
- (LynxFlexDirection)flexDirection;

// Now only supports computed length, length with auto and percentage will be 0
- (CGFloat)computedMarginLeft;
- (CGFloat)computedMarginRight;
- (CGFloat)computedMarginTop;
- (CGFloat)computedMarginBottom;
- (CGFloat)computedPaddingLeft;
- (CGFloat)computedPaddingRight;
- (CGFloat)computedPaddingTop;
- (CGFloat)computedPaddingBottom;
- (CGFloat)computedWidth;
- (CGFloat)computedHeight;
- (CGFloat)computedMinWidth;
// if max-width is not set, return LayoutNodeStyle::UNDEFINED_MAX_SIZE
- (CGFloat)computedMaxWidth;
- (CGFloat)computedMinHeight;
// if max-height is not set, return LayoutNodeStyle::UNDEFINED_MAX_SIZE
- (CGFloat)computedMaxHeight;

@end
