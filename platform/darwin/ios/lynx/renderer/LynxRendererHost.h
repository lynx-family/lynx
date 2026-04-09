// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxRenderer.h>
#import <UIKit/UIKit.h>

@class LynxRenderer;
@class LynxRendererContext;

@protocol LynxRendererHost <NSObject>

@property(nonatomic, strong) LynxRenderer *renderer;
@property(nonatomic, strong) LynxRendererContext *rendererContext;

- (instancetype)initWithRendererContext:(LynxRendererContext *)context;

- (LynxRenderer *)createRendererWithSign:(int32_t)sign andContext:(LynxRendererContext *)context;

- (UIView *)view;

@end
