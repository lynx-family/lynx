// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxRenderer.h>
#import <UIKit/UIKit.h>

@class LynxRenderer;

@protocol LynxRendererHost <NSObject>

- (void)setRenderer:(LynxRenderer*)renderer;

- (LynxRenderer*)createRendererWithSign:(int32_t)sign;

- (LynxRenderer*)getRenderer;

- (UIView*)getView;

@end
