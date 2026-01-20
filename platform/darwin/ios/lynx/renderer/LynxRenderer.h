// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol LynxRendererHost;

@interface LynxRenderer : NSObject

- (instancetype)initWithRenderHost:(UIView<LynxRendererHost> *)host andSign:(int32_t)sign;

- (int32_t)getSign;

@end
