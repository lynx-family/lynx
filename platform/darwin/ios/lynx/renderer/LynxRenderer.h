// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol LynxRendererHost;
@class LynxRendererContext;

@interface LynxRenderer : NSObject

- (instancetype)initWithRenderHost:(UIView<LynxRendererHost> *)host
                           andSign:(int32_t)sign
                        andContext:(LynxRendererContext *)context;

- (int32_t)sign;

- (UIView<LynxRendererHost> *)rendererHost;

- (LynxRendererContext *)context;

- (void)updateAttributes:(NSDictionary *)props;

- (void)updatePlatformExtraBundle:(id)data;

@end
