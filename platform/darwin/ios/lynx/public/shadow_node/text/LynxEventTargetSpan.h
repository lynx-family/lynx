// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxEventTarget.h"
#import "LynxEventTargetBase.h"
#import "LynxShadowNode.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxEventTargetSpan : NSObject <LynxEventTarget>

- (instancetype)initWithShadowNode:(LynxShadowNode*)node frame:(CGRect)frame;

- (void)setParentEventTarget:(id<LynxEventTarget>)parent;

@end

NS_ASSUME_NONNULL_END
