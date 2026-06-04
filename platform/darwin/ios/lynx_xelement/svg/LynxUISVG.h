// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>
#import <XElement/LynxSVGView.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxSVGRenderEngine;

@interface LynxUISVG : LynxUI <LynxSVGView *>

@property(nonatomic, strong, readonly) LynxSVGRenderEngine *engine;

// Deprecated: Use engine.src instead.
@property(nonatomic, copy, nullable)
    NSString *src DEPRECATED_MSG_ATTRIBUTE("Use engine.src instead.");
// Deprecated: Use engine.content instead.
@property(nonatomic, copy, nullable)
    NSString *content DEPRECATED_MSG_ATTRIBUTE("Use engine.content instead.");
// Deprecated: Use engine.color instead.
@property(nonatomic, copy, nullable)
    NSString *color DEPRECATED_MSG_ATTRIBUTE("Use engine.color instead.");

- (void)updateLayoutIfNeed;
- (UIImage *)processSVGData:(NSData *)data withSize:(CGSize)devSize;
- (UIImage *)loadImageFromHref:(NSString *)href withSize:(CGSize)devSize;

@end
NS_ASSUME_NONNULL_END
