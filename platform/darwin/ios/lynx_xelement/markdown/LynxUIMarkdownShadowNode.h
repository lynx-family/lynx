// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <Lynx/LynxCustomMeasureDelegate.h>
#import <Lynx/LynxShadowNode.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIMarkdownShadowNodeV2 : LynxShadowNode <LynxCustomMeasureDelegate>

- (NSString *)currentContentID;
- (void)setMarkdownContentOffset:(CGPoint)contentOffset;
- (BOOL)pauseAnimation;
- (BOOL)resumeAnimation:(NSInteger)animationStep;

@end

NS_ASSUME_NONNULL_END
